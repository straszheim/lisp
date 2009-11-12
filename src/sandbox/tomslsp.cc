/*
 * Tom's Lisp.
 *
 * This is a simple lisp interpreter.  Its purpose is not to be a feature-rich
 * standard, or especially efficient implementation, but to demonstrate 
 * principles of computer languages and of a lisp implementation.
 *
 * Copyright 2003, Thomas W Bennet
 * 
 * Tom's Lisp is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * Tom's Lisp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with the submission system distribution (see the file
 * COPYING); if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <typeinfo>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

using namespace std;

// *** Some compile configuration variables. ***

// The name of init file.
#ifndef INIT_FILE
#define INIT_FILE "tomsinit.lsp"
#endif

// The settings based on the target.
#ifdef WIN32

#define COMP_FOR "Win 32"

// File separator in paths.
#define SEP_CHAR ";"

// File separator in file names.
#define PATH_CHR "\\"

// The default search path for all file loads (windows)
#ifndef LSP_PATH
#define LSP_PATH ".;C:\\TOMSLSP;C:\\PROGRAM FILES\\TOMSLSP"
#endif

// Function to decide if a file name looks absolute (don't use the path).
inline bool isabs(const string fn)
{
	if(fn.length() < 3) return 0;
	return fn[0] == '.' || 
		(isupper(fn[0]) && fn[1] == ':' && fn[2] == '\\');
}

#else

#define COMP_FOR "Unix/Unix-like"

// File separator in paths.
#define SEP_CHAR ":"

// File separator in file names.
#define PATH_CHR "/"

// The default search path for all file loads.
#ifndef LSP_PATH
#define LSP_PATH ".:/usr/local/lib/tomslsp:/usr/lib/tomslsp"
#endif

// Function to decide if a file name looks absolute (don't use the path).
inline bool isabs(const string fn)
{
	return fn.length() && (fn[0] == '/' || fn[0] == '.');
}

#endif

// Also MEM_DEBUG if set adds checks related to debugging the ref count
// memory system.

#define VERSION "0.93"

/* Inheritence heirarchy.
 *
 *	RefPtr			       	      RefCtObj
 *	  |			              /     \
 *	  |			            /        \
 *	Ptr<T>    			Context  _ Evaluable _________
 * 	  |					/             \	      \
 *    	  |				       /               \       \
 *    DiscRefPtr	      _____________ Atom _          Applyable   Pair
 *			     /	    /    /   |    \           /      \
 *		           Error  Str  Id   Int   Nil      Closure   Builtin
 *							    /    \	  \
 *						  FuncClosure MacClosure  Catch
 */

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//***  Section I.  Interrupt facility.
//***
//***    This uses the signal facility to control set a flag when the user
//***    hits ^c.  The evalution method simply checks this flag and turns
//***    it into a lisp error when it has been set.
//***    
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

// Interrupt facilty.  Actually needed by apply.
bool signalled = false;		// A signal has been received.

// Vector which just sets the flag.
void whap(int sig) 
{
	signalled = true;
}

// This initializes the facility by asking the system to call whap() when
// the user hits ^c (SIGINT), or uses the kill command with no signal
// specified (SIGTERM is the default for the Unix kill command).
void initsig(void)
{
	signal(SIGINT, whap);
	signal(SIGTERM, whap);
}

// Test the flag, clearing it after the test.  This allows code to use
// if(zapped()) return Error() without having to worry about clearing the
// flag itself so it's not seen again.
bool zapped(void)
{
	bool oldsig = signalled;
	signalled = false;
	return oldsig;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//***  Section I.  Reference counting.
//***
//***    This section provides a reference counting facility which is used 
//***    to manage all dynamically-allocated memory in the system.
//***    
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

/* Nodes and pointers to nodes.  There should be no way for the client
   to get a Node object except by Node::alloc, so there's no way to
   allocate one outside the management of NodePtr.  */

/* This is a base for any reference-counted object.  It holds the count, and
   has utilities for maniuplating it.  There is no public interface.  The
   reference counts are manipulated by the derived class, and by the smart
   pointer class which is a friend. */
class RefCtObj {
#ifdef MEM_DEBUG
protected:
	// This is a count of the number of object allocated.  Used for
	// debuggin when MEM_DEBUG is set at compile time.
	static int alocnt;

	// Controls reporting of allocation and deallocation.
	static int alo_rpt;
#endif
private:
	// This class describes pointers to objects of RefCtObj.
	friend class RefPtr;
	friend class DiscRefPtr;

	// How many pointers point to me.
	int refct;

	// Equality.  The equal just returns false if the types
	// differ, but calls the virtual (type-specific) equal_same
	// if the types match.
	virtual bool equal_same(RefCtObj& other) { 
		return false; 
	}
	bool equal(RefCtObj&);

protected:
	// Find the count.
	int curr_refct() { return refct; }

	// Increase the count.
	virtual void mkref() { 
#ifdef MEM_DEBUG
		if(alo_rpt > 2) cout << "Incre " << this << ": count "
				     << refct << " -> " << refct + 1 << endl;
#endif
		++refct; 
	}

	// Decrement the reference count, delete self if needed.
	virtual void unref() 
	{
#ifdef MEM_DEBUG
		if(alo_rpt > 2) cout << "Decr " << this << ": count "
				     << refct << " -> " << refct - 1 << endl;
#endif
		if(--refct == 0)
			delete this;
	}

	// Create a new one with zero count
	RefCtObj(): refct(0) { 
#ifdef MEM_DEBUG
 		if(alo_rpt) cout << "Allocating " << this << " "
 				 << ": count "
 				 << alocnt << " -> " << alocnt + 1 << endl;
		++alocnt;
#endif
	}

	// Copy cons resets count.
	RefCtObj(const RefCtObj& otr): refct(0) {
#ifdef MEM_DEBUG
 		if(alo_rpt) cout << "Allocating " << this << " copy of "
 				 << &otr << " " << ": count "
 				 << alocnt << " -> " << alocnt + 1 << endl;
		++alocnt;
#endif
	}

	// Create a RefPtr from an plain pointer to an arbitrary RefCtObj.
	// This exists mainly because it uses the private constructor of
	// RefPtr, which can do because of friendship.  So this method is
	// really a way for our descendents to use the private constructor
	// in RefPtr.
	static RefPtr newRefPtr(RefCtObj *created);

	// Make a new ref to self.  Just newRefPtr(this).
	RefPtr newref();

#ifdef MEM_DEBUG
	// Count it gone.
	virtual ~RefCtObj() { 
		if(alo_rpt) cout << "Freeing " << this << ": count "
				 << alocnt << " -> " << alocnt - 1 << endl;
		--alocnt;
	}

public:
	// Report the number of blocks allocated.  Used for debugging.
	static void rpt(char *msg) { 
		cout << msg << ": " << alocnt << " blocks allocated." << endl; 
	}

	// Control the value of the allocation reporting variable.
	static void reporting(int b) { alo_rpt = b; }
#endif
};
#ifdef MEM_DEBUG
int RefCtObj::alocnt = 0;
int RefCtObj::alo_rpt = 0;
#endif

/* This is the smart pointer class.  It points to a type derived from
   RefCtObj, and uses its constructors to manage the reference count and
   delete objects as needed. */
class RefPtr {
	friend class RefCtObj;
protected:
	RefCtObj *target;		// The actual pointer.

	// Construct a reference to the object specifed by an ordinary ptr.
	RefPtr(RefCtObj *obj) {
		target = obj;
		if(target) target->mkref();
#ifdef MEM_DEBUG
		if(target && target->refct == 1) {
			if(RefCtObj::alo_rpt)
				cout << "Refed " << target << " " 
				     << typeid(*target).name() << endl;
 		} else {
			if(target && RefCtObj::alo_rpt > 1)
 				cout << "ReRef " << target << " " 
 				     << typeid(*target).name()
 				     << target->refct - 1 << " -> " 
 				     << target->refct << endl;
 		}
#endif
	}
public:
	// Public constructor creates the null pointer.
	RefPtr() { target = 0; }

	// Copy constructor increments the reference count of the object
	// pointed to.
	RefPtr(const RefPtr &r) {
		target = r.target;
		if(target) target->mkref();
#ifdef MEM_DEBUG
 		if(target && RefCtObj::alo_rpt > 1)
 			cout << "CopyRef " << target << " " 
 			     << typeid(*target).name() << " "
 			     << target->refct - 1 << " -> " 
 			     << target->refct << endl;
#endif
	}

	// Tell if this is the null pointer.
	bool isnull() { return target == 0; }

	// Set the pointer to null.  This reduces the reference count,
	// which may delete the object.
	void nullify() 
	{ 
#ifdef MEM_DEBUG
 		if(target && ((RefCtObj::alo_rpt && target->refct == 1) || 
 			      RefCtObj::alo_rpt > 1))
 			cout << "Nullify " << target << " " 
 			     << typeid(*target).name() << " "
 			     << target->refct << " -> " 
 			     << target->refct - 1 << endl;
#endif
		if(target) {
			target->unref();
		}
		target = 0;
	}

	// Assign from another pointer.  This will reduce the ref count of
	// what we are pointing to now, and increase that of what we will
	// now point to.
	void assign(const RefPtr lft)
	{
		nullify();
		target = lft.target;
		if(target) target->mkref();
#ifdef MEM_DEBUG
 		if(RefCtObj::alo_rpt > 1)
 			cout << "Assign " << target << " " 
 			     << typeid(*target).name()
 			     << target->refct - 1 << " -> " 
 			     << target->refct << endl;
#endif
	}
	RefPtr & operator=(const RefPtr & lft) {
		assign(lft);
		return *this;
	}

	// Destroy it.  This must reduce the ref count of the object denoted.
	~RefPtr() { 
#ifdef MEM_DEBUG
		if(RefCtObj::alo_rpt > 1 || 
		   (RefCtObj::alo_rpt == 1 && target && target->refct == 1))
			cout << "Destroy ptr to " << target << endl;
#endif

		nullify(); 
	}

	// Test the target type.  Use like p.points_to(typeid(Fred)), where
	// Fred is a class.
	bool points_to(const type_info &tid) {
		if(target == 0) return false;
		return tid == typeid(*target);
	}

	// For debugging.
	void prt(ostream &s) { s << target; }

	// For implementing the eq? operation.  Just compares the target ptrs.
	bool eq(const RefPtr &r) { return target == r.target; }

	// Test that we point to an object with the same value (equal) to
	// the one r points to.  Used to implement equal?
	bool same_value(const RefPtr &r) { 
		return target == r.target || target->equal(*r.target); 
	}
};

/* These bodies belong to RefCtObj, but need to follow RefPtr because they
   allocate RefPtr objects.  */
inline RefPtr RefCtObj::newRefPtr(RefCtObj *created) 
{
	return RefPtr(created);
}
inline RefPtr RefCtObj::newref()
{
	return newRefPtr(this);
}
inline bool RefCtObj::equal(RefCtObj& other) {
	if(typeid(*this) != typeid(other)) return false;
	else return equal_same(other);
}

/* Note: All the classes descended from RefCtObj contain a method like this
   for allocation:

	// Allocate one and return a pointer.
	static Ptr<Fred> Fred::alloc(whatever) {
		return RefCtObj::newRefPtr(new Fred(whatever));
	}

   This is a static member of class Fred which allocates a Fred object but
   returns a Ptr<Fred> to refer to it, rather than a plain C++ pointer to
   Fred.  Fred does not have a public constructor, so it should be 
   impossible to get a plain pointer to a Fred object. */

/* This is adaptor that makes the smart pointer a bit easier to use with
   classes derived from RefCtObj.  It represents a pointer to any type T
   which is derived form RefCtObj.  It also contains an overload of -> which
   allows the use of methods of type T using the p->f() notation. */
template <class T>
class Ptr: public RefPtr {
public:
	// Construct.
	Ptr(): RefPtr() { }
	Ptr(const RefPtr &r): RefPtr(r) { }

	// Assign.  
	Ptr & operator=(const RefPtr & lft) { 
		assign(lft);
		return *this;
	}

	// Allow -> on Ptr<T> objects.  
	T * operator->() { return dynamic_cast<T*>(target); }

	// This should happen automatically, but it doesn't seem to.
	//~Ptr() { nullify(); }
};

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//***  Section II.  Context.
//***
//***    A context is a stack of symbol tables.  These are used to store
//***    variables placed by setq.  A stack is used to control contexts.
//***    
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

/* A context is a stack of symbol tables.  Expressions are evaluated in some
   context, and function defintions (which are closures) use them. */
class Context: public RefCtObj {
	// Next context down.
	Ptr<Context> next;

	// Map itself -- holds the key->value items created by the
	// system or the (set ) builtin.
	map<string, RefPtr> content;
	typedef map<string, RefPtr>::iterator itype;

public:
	// Search for a symbol.  Goes down the stack until found.
	RefPtr find(const string &a) {
		itype loc = content.find(a);
		if(loc == content.end())
			if(next.isnull())
				return RefPtr();
			else
				return next->find(a);
		else
			return loc->second;
				
	}

	// Set a symbol
	void set(string a, RefPtr r) {
		content[a] = r;
	}

	// Allocate a base context.
	static Ptr<Context> alloc() {
		return RefCtObj::newRefPtr(new Context);
	}

	// Allocate a new context stacked above this one.
	Ptr<Context> scope() {
		Ptr<Context> ret = RefCtObj::newRefPtr(new Context);
		ret->next = newref();
		return ret;
	}

	// Compare for same type.
	virtual bool equal_same(RefCtObj& other) {
		return this == &other;
	}

	// Call this before removing a frame from the call stack.  If the
	// reference count is one, so that the pending removal will immediatly
	// delete the object, this does nothing.  Otherwise, it sets all
	// contained context pointers to non-supporting.
	void last_rites();
};

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//***  Section III.  Evaluable abstract class.
//***
//***    All the object visible to the interpreted program are Evaluable,
//***    though sometimes evaluating them will yield an error.  They also
//***    have car() and cdr() methods, like pairs, though they should not
//***    be called except from Pair (below).  It's convenient to have them
//***    here just to reduce the number of casts needed to get anything
//***    done.  (There's already a lot.)
//***    
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

/* These are objects on which the eval() method works.  These are exposed to
   the interpreted code. */
class Evaluable: public RefCtObj {
public:
	// These can be evaluated, they all have a name, and they can all
	// be printed.
	virtual Ptr<Evaluable> eval(Ptr<Context>) = 0;
	virtual string name() = 0;
	virtual void print(ostream &) = 0;

	// Some objects can be applied (to).  But not 
	// unless they really are . . . 
	virtual bool applyable() { return false; }

	// Nice to be able to find these, too.
	virtual bool closure() { return false; }

	// These really don't belong here in the heirarchy, but it makes
	// life *much* easier to be not constantly casting stuff.
	virtual Ptr<Evaluable> car() { 
		cerr << "*** Internal error: car() on non-list object. ***";
		abort();
	}
	virtual Ptr<Evaluable> cdr() { 
		cerr << "*** Internal error: cdr() on non-list object. ***";
		abort();
	}
};

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//***  Section IV.  Atoms.
//***
//***    Various types of atoms.  These represent the indivisible data 
//***    objects in the list system.  The classes for various atomic
//***    types tend to look a lot alike.
//***
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

// Atoms.  That is, data which has no parts.
class Atom: public Evaluable {
public:
	// All Evaluables can be printed and have a name, but for atoms,
	// printing usually just means printing the name.  This is 
	// overridden for some of them.
	virtual void print(ostream &s) { s << name(); }
};

// The nil atom.  Should create only one object of this class.  
class Nil: public Atom {
	Nil() { }
public:
	string name() { return "nil"; }
	static Ptr<Nil> alloc() {
		return RefCtObj::newRefPtr(new Nil);
	}

	// Evalution yields a pointer to the same object: nil evaluates
	// to itself.
	Ptr<Evaluable> eval(Ptr<Context>) { return newref(); }

	// Any Nil object is equal to any other (though unless there's a bug
	// somewhere, this can only be called to compare the nil object to
	// itself.)
	virtual bool equal_same(RefCtObj& other) { return true; }
};

// The unique Nil object (I hope).
static Ptr<Nil> the_nil = Nil::alloc();

// String constant atom.
class Str: public Atom {
protected:
	const string _val;		// Just the string this is.
	Str(string s): _val(s) { }
public:
	// The name is the value with quotes around it.
	string name() { 
		return string("\"") + _val + string("\"");
	}
	const string val() { return _val; }

	// Allocate one and return a pointer.
	static Ptr<Str> alloc(string s) {
		return RefCtObj::newRefPtr(new Str(s));
	}

	// This just allocates the string from the individual character, which
	// turns out to be convenient.  
	static Ptr<Str> alloc(char ch) {
		char s[] = { 0, 0 };
		s[0] = ch;
		return alloc(s);
	}

	// Evaluates to itself.
	Ptr<Evaluable> eval(Ptr<Context> c) { 
		return newref();
	}

	// Lisp equality is just the normal string comparison.
	virtual bool equal_same(RefCtObj& other) { 
		return _val == ((Str*)&other)->_val;
	}
};

/* This is an error result.  Each error result is a code number and a
   description.  The code number indicates the type of error, and the 
   description with each error code will vary a bit, mostly being more
   specific.  It operates as an exception, because any operation
   which sees an error object in any of its inputs will return it rather than
   performing its usual operation.  The function apply and list evaluation
   operators also add to the history when they pass an Error in this way,
   so when the error is finally printed it will contain a history of each
   operation which its presence aborted.  This forms essentially a call
   trace. */
class Error: public Atom {
	string _msg;		// Error message.
	int _code;		// Code number.
	Ptr<Evaluable> _hist;	// Error's history (list of evaluated forms)
	static bool _histon;	// Use the history feature.
	static int _toplim, _botlim;
				// Limits on history printing.

	// Simple constructor.  
	Error(int c, string m):
		_code(c), _msg(m), _hist(the_nil) { }

public:
	// Allocator.
	static Ptr<Error> alloc(int c, string m, Ptr<Evaluable> h = the_nil) 
	{
		Ptr<Error> e = RefCtObj::newRefPtr(new Error(c, m));
		if(!h.points_to(typeid(Nil))) e->hist(h);
		return e;
	}

	// Get code and string.
	int code() { return _code; }
	string msg() { return _msg; }

	// Name of the error, a description showing its code and string.
	virtual string name() {
		ostringstream strm;
		strm << _code;
		return string("Error ") +  strm.str() + ": " + _msg;
	}

	// This prints the error message.  Printing generally prints
	// not only the code and message, but the history (stack trace)
	// as well.
	virtual void print(ostream &s);

	// Evaluates to itself.
	Ptr<Evaluable> eval(Ptr<Context> c) { 
		return newref();
	}

	// Errors are considered equal if their codes are equal.  However,
	// as far as I know, there's not way to actually invoke this, 
	// because if you send equal? a couple of errors to compare, 
	// it will just return the first one.
	virtual bool equal_same(RefCtObj& other) {
		return _code == ((Error*)&other)->_code;
	}

	// Turn the history facilty on or off.
	static void history_off() { _histon = false; }
	static void history_on() { _histon = true; }
	static void toplim(int n) { _toplim = n; }
	static void botlim(int n) { _botlim = n; }

	// Add to the history.  This really just does something like
	// _hist = (e . _hist):  it adds e to the front of the object
	// variable _hist, which is just a lisp list.
	Ptr<Error> hist(Ptr<Evaluable> e);

	// This adds e to the front of the first first member of _hist.
	// Essentially: _hist = ((e . car(_hist)) . cdr(_hist)).
	Ptr<Error> histinc(Ptr<Evaluable> e);
};

// By default, the history feature is on.  Program opts turn it off.
bool Error::_histon = true;
int Error::_toplim = 5;
int Error::_botlim = 10;

// Print for an error object.  Prints the history, with output limits.
void Error::print(ostream &s)
{
	s << "**** " << name() << " ****";
	if(!_histon) return;

	// Print the first part, up to the limit.
	int toplim = _toplim;
	Ptr<Evaluable> i = _hist;
	for(; !i.points_to(typeid(Nil)); i = i->cdr()) {
		if(toplim-- == 0) break;
		cout << endl << "   "; 
		i->car()->print(cout);
	}

	// If we printed the whole list, we're done.
	if(i.points_to(typeid(Nil))) return;

	// Find where the second part starts.  The lag pointer waits until
	// i has moved _botlim ahead, then lag starts to move at the same
	// rate.
	Ptr<Evaluable> lag = i;
	int botlim = _botlim;
	for(; !i.points_to(typeid(Nil)); i = i->cdr())
		if(botlim-- <= 0) lag = lag->cdr();

	// Do the limits create a gap in the list?  If so, say so.
	if(botlim < 0) cout << endl << "   . . .";

	// Print the last part.
	for(i = lag; !i.points_to(typeid(Nil)); i = i->cdr()) {
		cout << endl << "   "; 
		i->car()->print(cout);
	}
}

// Establish code numbers and C++ variables for each type of
// builtin error.
int enct = 1;
int UNDEF = enct++;	// Undefined name.
int BADOP = enct++;	// Operation on inappropriate type.
int BADARG = enct++;	// Badly-formed argument list.
int SHORTARG = enct++;	// Missing arguments.
int BADTYPE = enct++;	// Bad argument type.
int LONGARG = enct++;	// Too many arguments to a builtin.
int RPAREN = enct++;	// Right paren expected.
int SYN = enct++;	// General syntax error.
int OPFAIL = enct++;	// Loaded file could not be openend.
int DIVZERO = enct++;	// Division by zero.
int INTER = enct++;	// Operation interrupted.

// Identifier atoms.
class Id: public Atom {
	// The string it looks like.
	const string _name;

	Id(const string s): _name(s) { }
public:
	string name() { return _name; }

	// Allocate one.
	static Ptr<Id> alloc(const string s) {
		return RefCtObj::newRefPtr(new Id(s));
	}

	// Evaluate by looking up in the current context.
	Ptr<Evaluable> eval(Ptr<Context> c) { 
		RefPtr v = c->find(_name);
		if(v.isnull())
			return Error::alloc(UNDEF,
					    string("Undefined ") + _name);
		else
			return v;
	}

	// They are equal if they have the same id (string name).  That will
	// matter if you run (equal? 'fred 'fred), which will be true, or
	// (equal? 'fred 'barney), which won't.  If you run 
	// (equal? fred barney), it will evaluate fred and barney (look them
	// up in the current context) and compare the results, which, in
	// most cases, won't call this at all.
	virtual bool equal_same(RefCtObj& other) { 
		return _name == ((Id*)&other)->_name;
	}

};

// This atom is used to return true results from tests.  It's just an 
// ordinary identifier, though.  Down in main() we set it to equal itself
// in the base context, so it will evaluate to itself, unless some fool
// changes the setting.
Ptr<Id> the_true = Id::alloc("#t");

// Integers.  Just uses the C++ integer.  (No unbounded integers like
// real lisps.)
class Int: public Atom {
	const int _val;
	Int(int v): _val(v) { }
public:
	// This mumbo-jumbo just converts the integer to a string, and
	// returns it.  There's probably an easier way to do this, but
	// I haven't found it yet.
	string name() { 
		ostringstream strm;
		strm << _val;
		return strm.str();
	}

	// Plain value.
	int val() { return _val; }

	// Standard allocator.
	static Ptr<Int> alloc(int v) {
		return RefCtObj::newRefPtr(new Int(v));
	}

	// Allocate from a string of digits.  This is needed mainly by
	// input routine.
	static Ptr<Int> alloc(string v) {
		istringstream is(v);
		int iv;
		is >> iv;
		return RefCtObj::newRefPtr(new Int(iv));
	}

	// Evaluates to itself.
	Ptr<Evaluable> eval(Ptr<Context> c) { 
		return newref();
	}

	// Equality is simple C++ language integer equality.
	virtual bool equal_same(RefCtObj& other) { 
		return _val == ((Int *)&other)->_val;
	}
};

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//***  Section V.  Applyable objects.
//***
//*** 
//***    These are objects which can be functions.  They include Pair, the 
//***    basic lisp data constructor, which is used to build functions (as
//***    well as other structures).  Also included are the builtin functions.
//***
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

/* These are objects which apply (with a parameter list and context) work. */
class Applyable: public Evaluable {
public:
	virtual bool applyable() { return true; }
	virtual Ptr<Evaluable> apply(Ptr<Evaluable>, Ptr<Context>) = 0;

	// Evaluates to itself.
	Ptr<Evaluable> eval(Ptr<Context> c) { 
		return newref();
	}
};

/* This is the pair (result of cons).  It represents a non-empty list. */
class Pair: public Evaluable {
	// The contents of a pair are pointers to two other things.
	Ptr<Evaluable> _car, _cdr;

private:
	// Create.  The public alloc() provides defaults of nil.
	Pair(Ptr<Evaluable> cd, Ptr<Evaluable> cr):
		_car(cd), _cdr(cr) { }
public:
	// Allocate a pair, return a (smart) pointer.
	static RefPtr alloc(Ptr<Evaluable> cd = the_nil, 
			    Ptr<Evaluable> cr = the_nil) {
		return RefCtObj::newRefPtr(new Pair(cd, cr));
	}

	// This evaluates the pair as a (func args), if possible.  Will
	// return an error atom if something goes wrong.
	Ptr<Evaluable> eval(Ptr<Context> c);

	// Print the list.  The two-argument form is essentially a helper
	// function for the regular print.  Listmode indicates the pair
	// should be printed as a list, otherwise it may be printed as a
	// dotted pair, though this can change to listmode.
	virtual void print(ostream &s, bool listmode);
	virtual void print(ostream &s) { print(s, false); }

	// Names should be short, even when printing is long.
	virtual string name() { return "A Pair"; }

	// The parts of the pair.
	virtual Ptr<Evaluable> car() { return _car; }
	virtual Ptr<Evaluable> cdr() { return _cdr; }

	// Equality means both sub-pointers point to equal things.
	virtual bool equal_same(RefCtObj& other) { 
		return _car.same_value(((Pair *)&other)->_car) && 
			_cdr.same_value(((Pair *)&other)->_cdr);
	}
};

/* Function evaluation.  Pair must be a function and an argument list.  This
   takes a list like (f x1 x2 x3 ... ), separates f, then applies 
   (x1 x2 x3 ... ) to f.  Note that if eval gets an error from any of the
   evaluations or applies, it returns that error, but it adds trace information
   to its history.  */
Ptr<Evaluable> Pair::eval(Ptr<Context> c) 
{
	// Eval the car (the function to run).  If this fails, add the
	// form which we were trying to evaluate, and pass the error on
	// up the stack.
	Ptr<Evaluable> func = car()->eval(c);
	if(func.points_to(typeid(Error)))
		return Ptr<Error>(func)->hist(this->newref());

	// Check that the function is really an applyable thing (such as the
	// the car builtin or a Closure, rather than a string or integer.)
	// If not, create an error here to reaturn.
	if(!func->applyable()) 
		return Error::alloc(BADOP, 
				    "Attempt to apply on " + func->name(),
				    newref());

	// Apply the arguments to the function.  If this fails, we use 
	// histinc() to add the name of whatever we were were applying
	// to the history.  We add that here because closures don't know
	// their own names (they're bound in the context, not contained
	// in the objects).  We let apply add the arguments themselves to
	// the history so we can get the evaluated versions.  That makes
	// a more useful trace, usually.
	Ptr<Evaluable> e = Ptr<Applyable>(func)->apply(cdr(), c);
	if(e.points_to(typeid(Error)))
		Ptr<Error>(e)->histinc(car());
	return e;
}

// Add to the error message history.  Needs to be located below pair.
inline Ptr<Error> Error::hist(Ptr<Evaluable> e)
{
	if(!_histon) return newref();
	_hist = Pair::alloc(e, _hist);
	return newref();
}

// Add to the front of the first item in the error message history.
inline Ptr<Error> Error::histinc(Ptr<Evaluable> e)
{
	if(!_histon) return newref();
	if(e.points_to(typeid(Nil))) return newref();
	_hist = Pair::alloc(Pair::alloc(e, _hist->car()), _hist->cdr());
	return newref();
}

/* Evaluate the members of list and return the new list.  If anything in the
   list fails, it return the error generated by the first such. */
Ptr<Evaluable> evalall(Ptr<Evaluable> lst, Ptr<Context> c) 
{
	// Test for empty.
	if(lst.points_to(typeid(Pair))) {
		// Not empty.

		// Evaluate the head of the list.  If bad, bug out.
		Ptr<Evaluable> ceval = lst->car()->eval(c);
		if(ceval.points_to(typeid(Error))) return ceval;

		// Recursively evaluate the tail of the list.  Pass up
		// an error if one occurs.
		Ptr<Context> recur = evalall(lst->cdr(), c);
		if(recur.points_to(typeid(Error))) return recur;

		// Construct the list of the evaluated head and tail.
		return Pair::alloc(ceval, recur);
	} else
		// Empty.

		// Evaluate the last thing, which is most likely Nil and
		// evaluates to itself.
		return lst->eval(c);
}

// This denotes a builtin functions.  It has no string representation 
// which is recognized on input, though all the ones we create are bound
// to names in the base context.
class Builtin: public Applyable {
protected:
	// This is a C++ function which actually performs the operation.
	// The declaration denotes a pointer to a function which returns a 
	// Ptr<Evaluable> and takes a Ptr<Evaluable> and a Ptr<Context>.
	// The first is an argument list, and the second is the context
	// in which the function is to be evaluated.
	Ptr<Evaluable> (*func)(Ptr<Evaluable>, Ptr<Context>);

	// The name is here only to make nice messages.  The inerpreter 
	// itself doesn't care.
	string _name;

	// Tell it should evaluate parameters first (normal function behavior),
	// or not (macro behavior).
	bool preval;

	// Signature for type check on apply.
	char * signature;

	// Construction.  Takes the name of the C++ implementation function
	// the name, a string denoting the correct signature (see sigchk),
	// and the evaluate-arguments boolean.  These are stored.
	Builtin(Ptr<Evaluable> (*r)(Ptr<Evaluable>, Ptr<Context>),
		string n, char * sig, bool p): 
		func(r), _name(n), preval(p), signature(sig) { }

	// Check an argument list against a signature specification.
	// Return nil or an error.  See function itself below for notes
	// on the specification language.
	Ptr<Evaluable> sigchk(Ptr<Evaluable> list, char *sig, char **term = 0);
	Ptr<Evaluable> sigchk(Ptr<Evaluable> list, 
			      char **term = 0) { 
		return sigchk(list, signature, term);
	}
public:
	// Allocator.  Looks complicated, but it just takes all the parms
	// and sends them to the constructor, which just stores them
	// in the object.
	static Ptr<Builtin> alloc(Ptr<Evaluable> 
				  (*r)(Ptr<Evaluable>,Ptr<Context>), 
				  string n, char * sig, bool p = true){
		return RefCtObj::newRefPtr(new Builtin(r, n, sig, p));
	}

	// Apply the args to the function.
	Ptr<Evaluable> apply(Ptr<Evaluable> p, Ptr<Context> c);

	// Tell if the parms are evaluated before running.
	bool function_like() { return preval; }
	bool macro_like() { return !preval; }

	// These are needed, but they don't  do much.
	virtual void print(ostream &s) { s << _name; }
	virtual string name() { return string("Builtin ") + _name; }

	// This seems pointless too, but needed for completeness.
	virtual bool equal_same(RefCtObj& other) { 
		return _name == ((Builtin*)&other)->_name && 
			func == ((Builtin*)&other)->func;
	}
};

// Apply the function to the argument list in the indicated context.
Ptr<Evaluable> Builtin::apply(Ptr<Evaluable> p, Ptr<Context> c)
{ 
	// Check for an interrupt.
	if(zapped()) return Error::alloc(INTER, "Interrupted.", p);

	// Perform argument evaluation if required.
	if(function_like()) {
		// Evaluate the arg list, pass up an error if one
		// appears.  Add the list of parms to the history of 
		// the error.
		Ptr<Evaluable> ep = evalall(p, c);
		if(ep.points_to(typeid(Error))) 
			return Ptr<Error>(ep)->hist(p);
		p = ep;
	}

	// Check that it was called correctly.
	Ptr<Evaluable> e = sigchk(p);
	if(e.points_to(typeid(Error)))
		return Ptr<Error>(e)->hist(p);

	// Okay.  NOW we can run the function.
	Ptr<Evaluable> ret = func(p, c); 
	if(ret.points_to(typeid(Error)))
		Ptr<Error>(ret)->hist(p);
	return ret;
}

/* The catch function requires special behavior because it actually 
   consumes an error argument.  Otherwise, it acts like a Builtin with
   bi_begin loaded as the operator function. */
Ptr<Evaluable> bi_begin(Ptr<Evaluable> e, Ptr<Context> c);
class Catch: public Builtin {

	// Construction.  We're not generalized, so we don't take much
	// from the client.
	Catch(): Builtin(bi_begin, "catch", "+", false) { }
public:
	// Again, how we allocate.
	static Ptr<Catch> alloc() {
		return RefCtObj::newRefPtr(new Catch());
	}

	// Apply for catch.  Uses the base class to do an ordinary evalution
	// using begin semantics.  If that returns a non-error, we return
	// (#t . the-non-error-value).  If it goes boom, we build our
	// return value as (nil . ( error-code . error-message))
	Ptr<Evaluable> apply(Ptr<Evaluable> p, Ptr<Context> c) {
		// Perform an ordinary begin evaluation.
		Ptr<Evaluable> res = Builtin::apply(p, c);

		// Boom?
		if(res.points_to(typeid(Error)))
			// If it went boom, return a description of the
			// error.
			return Pair::alloc(the_nil, 
			    Pair::alloc(Int::alloc(Ptr<Error>(res)->code()),
				   Str::alloc(Ptr<Error>(res)->msg())));
	   	else
			// If completed w/o error, return a pair to indicate
			// that, and the value produced.
			return Pair::alloc(the_true, res);
	}
};

/* This is a modified version of the smart pointer that allows a pointer to
   be "discounted," which means it is not included in the ref counts of what
   it points to.  This is needed for breaking the context-closure-context
   cycle.  I only need this for Contexts, so it's derived from Ptr<Context>. */
class DiscRefPtr: public Ptr<Context> {
	bool discounted;

	// Copying and assignment are not allowed.  This is for one
	// special use, and I really don't want it used for much.
	DiscRefPtr(DiscRefPtr &) { }
	void operator=(DiscRefPtr &) { }
public:
	// Construct naked or from the more standard variety.
	DiscRefPtr(): Ptr<Context>(), discounted(false) { }
	DiscRefPtr(RefPtr &r): Ptr<Context>(r), discounted(false) { 
#ifdef MEM_DEBUG
		if(target && RefCtObj::alo_rpt > 1)
			cout << "disc. copy " << " " << this << endl;
#endif
	}

	// Assignment from a normal RefPtr.
	DiscRefPtr & operator=(const RefPtr & lft) { 
#ifdef MEM_DEBUG
		if(target && RefCtObj::alo_rpt > 1)
			cout << "disc. assign to " << " " << this << endl;
#endif
		if(discounted && target) target->mkref();
		assign(lft);
		discounted = false;
		return *this;
	}

	// Remove from the target count.
	void discount() {
		if(discounted) return;
#ifdef MEM_DEBUG
		if(target && RefCtObj::alo_rpt > 1)
			cout << "discount " << " " << this << endl;
#endif
		discounted = true;
		if(target) target->unref();
	}

	// Return to count.
	void recount() {
		if(!discounted) return;
#ifdef MEM_DEBUG
		if(target && RefCtObj::alo_rpt > 1)
			cout << "reount " << " " << this << endl;
#endif
		discounted = false;
		if(target) target->mkref();
	}

	// Discounted pointers need to prevent the main destructor from 
	// decrementing the ref count again.
	~DiscRefPtr() {
		if(discounted) target = 0;
	}
};

/* A Closure is code and a context in which to run it.  This is still and
   abstract class; FuncClosure and MacClosure are derived from it for 
   functional (lambda) and macro closures, respectively. */
class Closure: public Applyable {
protected:
	Ptr<Evaluable> code;		// Code to be run.
	Ptr<Evaluable> parms;		// Parameter list.
	DiscRefPtr context;		// Context from creation.
	Closure(Ptr<Evaluable> p, Ptr<Evaluable> c, Ptr<Context> cn): 
		code(c), parms(p), context(cn), nonsupp(0) { }

	// Count the number of non-supporting pointers.  Non-supporting
	// pointers are ones stored in an exited context.  When all of our
	// pointers are non-supporting, we discount our context pointer.
	// When that changes, we recount it.
	friend class Context;
	int nonsupp;
	void nonsup() {
#ifdef MEM_DEBUG
		if(alo_rpt) 
			cout << "Nonsup " << this << nonsupp << " -> "
			     << nonsupp + 1 << endl;
#endif
		if(++nonsupp >= curr_refct()) context.discount();
	}

	// This extends the ref count mech to deal with the nonsupp.
	virtual void mkref() 
	{
#ifdef MEM_DEBUG
		if(RefCtObj::alo_rpt > 1)
			cout << "Closure::mkref() " << this << " "
			     << nonsupp << endl;
#endif
		RefCtObj::mkref();
		if(nonsupp < curr_refct()) context.recount();
	}
	virtual void unref() 
	{
#ifdef MEM_DEBUG
		if(RefCtObj::alo_rpt > 1)
			cout << "Closure::unref() " << this << " "
			     << nonsupp << endl;
#endif
		if(nonsupp >= curr_refct() - 1) context.discount();
		RefCtObj::unref();
	}

	// Print designed to be used by both closures.
	virtual void print(ostream &strm, char *type) { 
		strm << "(" << type << " ";
		parms->print(strm);
		strm << " ";
		code->print(strm);
		strm << ")";
	}

	// This is most of the code for apply for both function and
	// macro closures.  It pushes a new scope on the context cont, 
	// sets each parameter name with the value from the argument
	// list args, then evaluates the body in this new context.
	virtual Ptr<Evaluable> apply_guts(Ptr<Evaluable> args, 
					  Ptr<Context> cont);
public:
	// Equality is just equality of all parts.
	virtual bool equal_same(RefCtObj& other) { 
		return code.same_value(((Closure*)&other)->code) &&
			parms.same_value(((Closure*)&other)->code) &&
			context.same_value(((Closure*)&other)->context);
	}

	// Nice to be able to find these, too.
	virtual bool closure() { return true; }
};

// These are here because they calls a method from Closure.  They set all 
// Closure pointers inside the context to be non-supporting or supporting.
void Context::last_rites() 
{
	if(curr_refct() <= 1) return;
	for(itype p = content.begin(); p != content.end(); ++p)
		if(Ptr<Evaluable>(p->second)->closure())
			Ptr<Closure>(p->second)->nonsup();
}

// This contains most of the work for apply in MacClosure and FuncClosure.
Ptr<Evaluable> Closure::apply_guts(Ptr<Evaluable> p, Ptr<Context> c) 
{
	// Check for an interrupt.
	if(zapped()) return Error::alloc(INTER, "Interrupted.");

	// Get the parms.
	Ptr<Evaluable> parms = this->parms;

	// If the parms are a single identifier instead of a list of
	// identifiers, we bind the whole argument list to the single
	// id.  The easiest way to do this is to pretend we have one
	// ordinary parameter and were sent a single list argument.
	// That is, we wrap each one in an additional level of list.
	if(parms.points_to(typeid(Id))) {
		parms = Pair::alloc(parms, the_nil);
		p = Pair::alloc(p, the_nil);
	}

	// Scan the argument and parameter lists and add the parameters to
	// the context with their proper values.
	Ptr<Context> cont = this->context->scope();
	while(!parms.points_to(typeid(Nil))) {
		// Make sure the argument exists.
		if(p.points_to(typeid(Nil))) {
			cont->last_rites();
			return Error::alloc(SHORTARG, "Missing argument.");
		}

		// Make sure the list is well-formed.
		if(!p.points_to(typeid(Pair))) {
			cont->last_rites();
			return Error::alloc(BADARG, "Bad argument list.");
		}

		// Get the argument.
		RefPtr value = p->car();
		p = p->cdr();

		// Check for a boom.
		if(value.points_to(typeid(Error))) {
			cont->last_rites();
			return value;
		}

		// Find the name, and move the ptr.
		cont->set(Ptr<Id>(parms->car())->name(), value);
		parms = parms->cdr();
	}

	// Check for extras.
	if(!p.points_to(typeid(Nil))) {
		cont->last_rites();
		return Error::alloc(LONGARG, "Too many arguments");
	}

	// Evaluate.
	Ptr<Evaluable> v = code->eval(cont);
	cont->last_rites();
	return v;
}

/* Closure for a function. */
class FuncClosure: public Closure {
	// It would be nice if there were some way to ask to inherit
	// a constructor.
	FuncClosure(Ptr<Evaluable> p, Ptr<Evaluable> c, Ptr<Context> cn): 
		Closure(p, c, cn) { }
public:
	// Make one.
	static Ptr<FuncClosure> alloc(Ptr<Evaluable> p, 
				  Ptr<Evaluable> c, Ptr<Context> cn) {
		Ptr<FuncClosure> ret =
			RefCtObj::newRefPtr(new FuncClosure(p, c, cn));
		return ret;
	}

	// Apply the closure.  This means to evaluate the arguments p in the
	// calling context c, then evaluate the body in its stored context
	// with an added scope containing the arguments.  Return the result.
	virtual Ptr<Evaluable> apply(Ptr<Evaluable> p, Ptr<Context> c) {
		// Evaluate the parameters, bail on fail.
		Ptr<Evaluable> ep = evalall(p, c);
		if(ep.points_to(typeid(Error))) 
			return Ptr<Error>(ep)->hist(p);

		// Peform the application on the parameters.
		Ptr<Evaluable> res = Closure::apply_guts(ep, c);
		if(res.points_to(typeid(Error))) 
			Ptr<Error>(res)->hist(ep);

		// Send it back.
		return res;
	}

	virtual string name() { return "[a closure]"; }
	virtual void print(ostream &strm) { Closure::print(strm, "lambda"); }
};

/* Closure for a macro. */
class MacClosure: public Closure {
	// It would be nice if there were some way to ask to inherit
	// a constructor.
	MacClosure(Ptr<Evaluable> p, Ptr<Evaluable> c, Ptr<Context> cn): 
		Closure(p, c, cn) { }
public:
	// Make one.
	static Ptr<MacClosure> alloc(Ptr<Evaluable> p, 
				  Ptr<Evaluable> c, Ptr<Context> cn) {
		Ptr<MacClosure> ret =
			RefCtObj::newRefPtr(new MacClosure(p, c, cn));
		return ret;
	}

	// Apply the closure.  This means to evaluate the arguments p in the
	// calling context c, then evaluate the body in its stored context
	// with an added scope containing the arguments.  Return the result.
	virtual Ptr<Evaluable> apply(Ptr<Evaluable> a, Ptr<Context> c)
	{
		Ptr<Evaluable> res = apply_guts(a, c)->eval(c);
		if(res.points_to(typeid(Error)))
			Ptr<Error>(res)->hist(a);
		return res;
	}

	// Fillers, really.
	virtual string name() { return "[a macro]"; }
	virtual void print(ostream &strm) { Closure::print(strm, "macro"); }
};

// Argument signature checker.  Check the argument list against a signature
// specification and return nil or an error.  Signature expressions use single
// letters for types:
//    .   No restriction
//    l   List:  Pair or Nil
//    p   Pair
//    i   Int
//    s   Str
//    n   Id (name)
//    F   Mean a lambda or macro parameter argument.  Either a 
//  	  single identifier, or a list of identifiers.
//    +   Means zero or more.  + followed by one of the above means
//        all of them must have that type.
//    ()  Parenthesised subexpressions match lists with the form of
//        their contents.
// The signature which is checked defaults to the one for this
// object, but can be sent.  The term parameter is used to return
// the scan location after recursive calls, which are made to 
// process sublist specs in ()s.  It may be sent at NULL (the
// default) if this return is not desired, and valid data is
// returned only when the main return value is not an Error.  Since
// format strings are not input data, their form is not checked
// carefully.
//
// The term, if specified, returns the location in the sig at the
// end of the check, if the caller wishes that reported.  The
// sig is sent when the stored one is not desired, and it is used
// only for sigchk's own recursive calls.
Ptr<Evaluable> Builtin::sigchk(Ptr<Evaluable> e, char *sig, char **term)
{
	// Scan the list.
	while(1) {
		// End of list.  Either we're done, or the list was short.
		if(e.points_to(typeid(Nil)))
			if(*sig == '\0' || *sig == ')' || *sig == '+')
				break;
			else
				return Error::alloc(SHORTARG, _name + 
						    ": Missing argument.");

		// Not end of list, rest must be properly formed.
		if(!e.points_to(typeid(Pair)))
			return Error::alloc(BADARG, _name + ": Bad arg list.");

		// Since we're not out of parms, the spec must allow it.
		if(*sig == '\0' || *sig ==')')
			return Error::alloc(LONGARG, 
					    _name + ": Too many arguments.");

		// Get the character which specifies the check for this arg,
		// and advance the sig scanner.
		char curspec;
		if(*sig == '+')
			if(sig[1])
				curspec = sig[1];
			else
				curspec = '.';
		else
			curspec = *sig++;

		// Isolate first arg and move the list pointer down.
		Ptr<Evaluable> arg = e->car();
		e = e->cdr();

		// If the parameter is an error, that is the result of
		// the operation.
		if(arg.points_to(typeid(Error))) return arg;

		// Check type as appropriate.
		switch(curspec)
		{
		case '.':
			// No restriction.
			break;
		case 'l':
			// A pair or nil (list).
			if(!arg.points_to(typeid(Pair)) &&
			   !arg.points_to(typeid(Nil)))
				return Error::alloc(BADTYPE,
						    _name + ": Bad arg type.");
			break;
		case 'p':
			// A pair strictly.
			if(!arg.points_to(typeid(Pair)))
				return Error::alloc(BADTYPE, 
						    _name + ": Bad arg type.");
			break;
		case 'i':
			// Must be integer.
			if(!arg.points_to(typeid(Int)))
				return Error::alloc(BADTYPE, 
						    _name + ": Bad arg type.");
			break;
		case 's':
			// Must be string.
			if(!arg.points_to(typeid(Str)))
				return Error::alloc(BADTYPE, 
						    _name + ": Bad arg type.");
			break;
		case 'n':
			// Must be id (name).
			if(!arg.points_to(typeid(Id)))
				return Error::alloc(BADTYPE, 
						    _name + ": Bad arg type.");
			break;
		case 'F':
			// Either a single id or a lists of id's.  Needed 
			// for lambda and macro.
			if(!arg.points_to(typeid(Nil)) && 
			   !arg.points_to(typeid(Id))) {
				// If it's not a single ID or an empty list,
				// then it must be a non-empty list...
				if(!arg.points_to(typeid(Pair)))
					return Error::alloc(BADTYPE, _name + 
							    ": Bad arg type.");

				// ... and must consist only of names.
				Ptr<Evaluable> ilist = sigchk(arg, "+n");
				if(ilist.points_to(typeid(Error)))
					return ilist;
			}
			break;
		case '(':
			// Sublist.
			Ptr<Evaluable> slc = sigchk(arg, sig, &sig);
			if(slc.points_to(typeid(Error))) return slc;
			break;
		otherwise:
			cerr << "Internal error: Illegal signature " <<
				"string character." << endl;
			exit(3);
		}
	}

	// Normal return.
	if(term) {
		// Need to clear to proper place.
		while(*sig && *sig != ')') ++sig;
		if(*sig) ++ sig;
		*term = sig;
	}
	return the_nil;
}

// Following are functions which perform the builtin operations.  Each one
// is named bi_whatever, where whatever is the name of the builtin Lisp
// function it implements.

// *** Fundamental list operations ***
Ptr<Evaluable> bi_car(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car()->car();
}

Ptr<Evaluable> bi_cdr(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car()->cdr();
}

Ptr<Evaluable> bi_cons(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Pair::alloc(e->car(), e->cdr()->car());
}

// *** Tests.  Actual names have ? on them in lisp. ***
Ptr<Evaluable> bi_null(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().points_to(typeid(Nil)) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_ispair(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().points_to(typeid(Pair)) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_isid(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().points_to(typeid(Id)) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_isint(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().points_to(typeid(Int)) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_isstring(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().points_to(typeid(Str)) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_islambda(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().points_to(typeid(FuncClosure)) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_ismacro(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().points_to(typeid(MacClosure)) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_isbuiltin(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().points_to(typeid(Builtin)) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

// *** Evaluation ***

// Evaluate and return the arg.
Ptr<Evaluable> bi_eval(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car()->eval(c);
}

// Quote.
Ptr<Evaluable> bi_quote(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car();
}

// *** Control ***

// Evaluate all forms, return the last.  This is also used by
// class Catch, which is essentially a begin which treats errors
// differently.
Ptr<Evaluable> bi_begin(Ptr<Evaluable> e, Ptr<Context> c)
{
	Ptr<Evaluable> res = the_nil;
	while(e.points_to(typeid(Pair))) {
		res = e->car()->eval(c);
		if(res.points_to(typeid(Error))) return res;
		e = e->cdr();
	}
	return res;
}

// Traditional cond operator
Ptr<Evaluable> bi_cond(Ptr<Evaluable> e, Ptr<Context> c)
{
	while(!e.points_to(typeid(Nil))) {
		// List form.
		if(!e.points_to(typeid(Pair)))
			return Error::alloc(BADARG, "Bad cond form.");

		// Consider this case.
		Ptr<Evaluable> thiscase = e->car();
		e = e->cdr();

		// Looking for that end default
		if(e.points_to(typeid(Nil)) && 
		   !thiscase.points_to(typeid(Pair)))
			return thiscase->eval(c);

		// Regular case, must be a pair.
		if(!thiscase.points_to(typeid(Pair)))
			return Error::alloc(BADARG, "Bad cond form.");

		// Evaluate the test.
		Ptr<Evaluable> test = thiscase->car()->eval(c);
		if(test.points_to(typeid(Error))) return test;
		if(!test.points_to(typeid(Nil)))
			return thiscase->cdr()->car()->eval(c);
	}
	return the_nil;
}

// Return the first to evaluate non-nil
Ptr<Evaluable> bi_or(Ptr<Evaluable> e, Ptr<Context> c)
{
	if(e.points_to(typeid(Nil))) return the_nil;
	Ptr<Evaluable> v = e->car()->eval(c);
	if(v.points_to(typeid(Nil))) return bi_or(e->cdr(), c);
	return v;
}

// This returns the error object, as specified.  Essential a throw.
Ptr<Evaluable> bi_error(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Error::alloc(Ptr<Int>(e->car())->val(), 
			    Ptr<Str>(e->cdr()->car())->val());
}

// Time to go.
Ptr<Evaluable> bi_exit(Ptr<Evaluable> e, Ptr<Context> c)
{
	exit(Ptr<Int>(e->car())->val());
}

// *** Definition and Context ***

// Set a name.
Ptr<Evaluable> bi_set(Ptr<Evaluable> e, Ptr<Context> c)
{
	c->set(Ptr<Id>(e->car())->name(), e->cdr()->car());
	return e->car();
}

// This evaluates its contents in a new scope.
Ptr<Evaluable> bi_scope(Ptr<Evaluable> e, Ptr<Context> c)
{
	Ptr<Context> newcont = c->scope();
	Ptr<Evaluable> ret = bi_begin(e, newcont);
	newcont->last_rites();
	return ret;
}

// *** Closures ***
Ptr<Evaluable> bi_macro(Ptr<Evaluable> e, Ptr<Context> c)
{
	return MacClosure::alloc(e->car(), e->cdr()->car(), c);
}
Ptr<Evaluable> bi_lambda(Ptr<Evaluable> e, Ptr<Context> c)
{
	return FuncClosure::alloc(e->car(), e->cdr()->car(), c);
}

// *** Arithmetic ***
Ptr<Evaluable> bi_plus(Ptr<Evaluable> e, Ptr<Context> c)
{
	int sum = 0;

	while(!e.points_to(typeid(Nil))) {
		sum += Ptr<Int>(e->car())->val();
		e = e->cdr();
	}

	return Int::alloc(sum);
}
Ptr<Evaluable> bi_times(Ptr<Evaluable> e, Ptr<Context> c)
{
	int prod = 1;

	while(!e.points_to(typeid(Nil))) {
		prod *= Ptr<Int>(e->car())->val();
		e = e->cdr();
	}

	return Int::alloc(prod);
}
Ptr<Evaluable> bi_minus(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Int::alloc(Ptr<Int>(e->car())->val() -
			  Ptr<Int>(e->cdr()->car())->val());
}
Ptr<Evaluable> bi_div(Ptr<Evaluable> e, Ptr<Context> c)
{
	int divor = Ptr<Int>(e->cdr()->car())->val();
	if(divor == 0)
		return Error::alloc(DIVZERO, "Division by zero.");
	else
		return Int::alloc(Ptr<Int>(e->car())->val() / divor);
}
Ptr<Evaluable> bi_mod(Ptr<Evaluable> e, Ptr<Context> c)
{
	int divor = Ptr<Int>(e->cdr()->car())->val();
	if(divor == 0)
		return Error::alloc(DIVZERO, "Division by zero.");
	else
		return Int::alloc(Ptr<Int>(e->car())->val() % divor);
}

// Numerical comparison.
Ptr<Evaluable> bi_lt(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Ptr<Int>(e->car())->val() < Ptr<Int>(e->cdr()->car())->val() ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_gt(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Ptr<Int>(e->car())->val() > Ptr<Int>(e->cdr()->car())->val() ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_le(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Ptr<Int>(e->car())->val() <= Ptr<Int>(e->cdr()->car())->val() ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_ge(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Ptr<Int>(e->car())->val() >= Ptr<Int>(e->cdr()->car())->val() ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_eq(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Ptr<Int>(e->car())->val() == Ptr<Int>(e->cdr()->car())->val() ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

Ptr<Evaluable> bi_ne(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Ptr<Int>(e->car())->val() != Ptr<Int>(e->cdr()->car())->val() ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

// Plain eq?
Ptr<Evaluable> bi_ptreq(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().eq(e->cdr()->car()) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

// Recursive equal?
Ptr<Evaluable> bi_equal(Ptr<Evaluable> e, Ptr<Context> c)
{
	return e->car().same_value(e->cdr()->car()) ? 
		Ptr<Evaluable>(the_true) : Ptr<Evaluable>(the_nil);
}

// String operations.  There's enough here to make user-level operations
// using lists or characters, though this will not be particularly efficient.

// Well, this isn't strictly necessary, but I couldn't quite bear to pay the
// overhead of computing length with a shatter.
Ptr<Evaluable> bi_strlen(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Int::alloc(Ptr<Str>(e->car())->val().length());
}

// Turn a string into a list of one-character strings.
Ptr<Evaluable> shatter(const char *str) 
{
	if(*str == 0)
		return the_nil;
	else
		return Pair::alloc(Str::alloc(*str), shatter(str+1));
}
Ptr<Evaluable> bi_shatter(Ptr<Evaluable> e, Ptr<Context> c)
{
	return  shatter(Ptr<Str>(e->car())->Str::val().c_str());
}

// Take a list of strings and append them all together into one string.
string collect(Ptr<Evaluable> e)
{
	if(e.points_to(typeid(Nil)))
		return string("");
	else
		return Ptr<Str>(e->car())->val() + collect(e->cdr());
}
Ptr<Evaluable> bi_collect(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Str::alloc(collect(e->car()));
}

// Chr and ord.  Ord works on the first character of a string, and returns
// nil for the empty string.
Ptr<Evaluable> bi_chr(Ptr<Evaluable> e, Ptr<Context> c)
{
	return Str::alloc((char)(Ptr<Int>(e->car())->val()));
}
Ptr<Evaluable> bi_ord(Ptr<Evaluable> e, Ptr<Context> c)
{
	if(Ptr<Str>(e->car())->val().length() > 0)
		return Int::alloc(Ptr<Str>(e->car())->val()[0]);
	else
		return the_nil;
}

// Print whatever.
Ptr<Evaluable> bi_print(Ptr<Evaluable> e, Ptr<Context> c)
{
	e->car()->print(cout);
	return e->car();
}

// Print a string w/o quotes.  Useful for building more powerful 
// printing constructs.
Ptr<Evaluable> bi_sprint(Ptr<Evaluable> e, Ptr<Context> c)
{
	cout << Ptr<Str>(e->car())->val();
	return e->car();
}

// Load a file.  Includes the proto for the load() function from the
// i/o section which does most of the work.  The bi_load() is really an
// adaptor.
Ptr<Evaluable> load(string name, Ptr<Context> c);
Ptr<Evaluable> bi_load(Ptr<Evaluable> e, Ptr<Context> c)
{
	return load(Ptr<Str>(e->car())->val(), c);
}

#ifdef MEM_DEBUG
// Report the memory allocation count.
Ptr<Evaluable> bi_memrpt(Ptr<Evaluable> e, Ptr<Context> c)
{
	RefCtObj::rpt("Memory");
	return the_nil;
}

// Set the alloc/dealloc flag
Ptr<Evaluable> bi_aloc_log(Ptr<Evaluable> e, Ptr<Context> c)
{
	RefCtObj::reporting(Ptr<Int>(e->car())->val());
	return the_nil;
}
#endif

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//***  Section VI.  Input.
//***
//***    Functions and objects to read and print lisp lists.
//***
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

// Pair printing function.
void Pair::print(ostream &s, bool listmode)
{
	if(!listmode) s << '(';
	if(car().isnull()) s << "[null]";
	else car()->print(s);
	if(cdr().isnull()) s << " . [null]";
	else {
		// Print the cdr in some form.
		if(cdr().points_to(typeid(Pair))) {
			s << " ";
			(Ptr<Pair>(cdr()))->print(s, true);
		}
		else if(!cdr().points_to(typeid(Nil))) {
			s << " . ";
			cdr()->print(s);
		}

	}
	if(!listmode) s << ')';
}

// Token types.
enum tok_t { tok_id, tok_int, tok_str, tok_left, tok_right, tok_period,
	     tok_eof, tok_tick };

// Token values.
struct token {
	tok_t type;
	string text;
	token(tok_t tt = tok_eof, string txt = ""): type(tt), text(txt) { }
};

// Scanner objects.
class Scanner {
	istream &strm;		// Stream which is the character source.
	int lno;		// Current line number.
	char lastin;		// Last character read.(one-char lookahead).
	token _currtok;		// Current token.
	int nest;		// Current paren nesting depth.
	ostream *prompt;	// Promt stream.
	bool ahead;		// Scanner is "ahead" -- _currtok contains
				// valid data.
	string _filename;	// File name (for err report only).

	// Get the next character.
	bool getchr() {
		if(lastin == '\n') {
			++lno;
			if(prompt)
				if(nest <=  0)
					*prompt << "lsp>";
				else
					*prompt << "--->";
		}
		if(strm.get(lastin)) 
			return true;

		lastin = ' ';
		return false;
	}

	// Read the next token.
	token readtok()
	{
		// This loop scans through blanks and comments.
		while(1) {
			// Look for a non-blank.
			while(isspace(lastin))
				if(!getchr()) return token(tok_eof);

			// If that non-blank was a comment designator,
			// munch the line.
			if(lastin == ';') {
				while(lastin != '\n')
					if(!getchr()) return token(tok_eof);
				getchr();
			} else
				// Process it.
				break;
		}

		char first = lastin;
		getchr();

		// Now for some simple ones.
		switch(first)
		{
		case '(': 
			++nest;
			return token(tok_left, "(");
		case ')': 
			--nest;
			return token(tok_right, ")");
		case '.': 
			return token(tok_period, ".");
		case '\'': 
			return token(tok_tick, "'");
		case '"':
			string ret = "";
			while(1) {
				if(lastin == '"' || lastin == '\n') break;
				else if(lastin == '\\') {
					getchr();
					char inch = lastin;
					if(lastin == 'n') inch = '\n';
					else if(lastin == 't') inch = '\t';
					else if(lastin == '\n') inch = ' ';
					ret += inch;
				} else
					ret += lastin;
				if(!getchr()) break;
			}
			getchr(); // Clear the closing " 
			return token(tok_str, ret);
		}

		// Whatever's left is allowed as an identifier character or
		// a digit.  Accumulate it, and recall if it's all digits.
		string str = " ";
		str[0] = first;
		bool digits = isdigit(first);
		while(1) {
			if(isspace(lastin) || lastin == '(' || lastin == ')' ||
			   lastin == '.' || lastin == ';' || lastin == '\'' ||
			   lastin == '"')
				break;
			if(!isdigit(lastin)) digits = false;
			str += lastin;
			if(!getchr()) break;
		}

		// One kind or another.
		if(digits) return token(tok_int, str);
		else return token(tok_id, str);
	}

public:
	// Initialize the object.  Set set state and read one character.
	Scanner(istream &is, string fn, ostream *pr = 0): 
		strm(is), _filename(fn), prompt(pr), lastin('\n'), lno(0),
		ahead(false), nest(0) { }

	// Current token (does not change input stream).
	token peek() { 
		if(!ahead) {
			_currtok = readtok();
			ahead = true;
		}
		return _currtok; 
	}

	// Return current token, then go to the next one.
	token next() { 
		token retval = peek();
		ahead = false;
		return retval;
	}

	// Current line number.
	int line() { return lno; }

	// Current filename.
	string filename() { return _filename; }

	// Current location (for error reports).
	string location() {
		ostringstream strm;
		strm << _filename << ":" << lno << ends;
		return strm.str();
	}
	
	// Clear to end of line.
	void clear()
	{
		while(lastin != '\n') if(!getchr()) return;
		getchr();
		nest = 0;
		ahead = false;
	}

	// Clear until the nesting depth is zero or negative, then clear 
	// to the end of the line.  Used to clean up after an error.
	void purge() 
	{
		while(nest > 0) if(next().type == tok_eof) break;
		clear();
	}

	// EOF detect on the underlying stream.  
	bool eof() { return !strm.good(); }
};

// Read an s-expression.  At EOF, returns the NULL pointer (not to be confused
// with Nil, a perfectly valid input.)
Ptr<Evaluable> read_rest_list(Scanner &s);
Ptr<Evaluable> read_sexpr(Scanner &s)
{
	// Now do something with it.
	token t = s.next();
	if(t.type == tok_eof) return RefPtr();
	if(t.type == tok_id) return Id::alloc(t.text);
	if(t.type == tok_int) return Int::alloc(t.text);
	if(t.type == tok_str) return Str::alloc(t.text);
	if(t.type == tok_tick) {
		// Tick quotes the next whatever.  Read it, and
		// insert the quote atom.
		Ptr<Evaluable> e = read_sexpr(s);
		if(e.points_to(typeid(Error))) return e;
		return Pair::alloc(Id::alloc("quote"), 
				   Pair::alloc(e, the_nil));
	}
	if(t.type == tok_left) {
		// Left paren.  Look recursively for a sublist.
		Ptr<Evaluable> e = read_rest_list(s);
		if(s.peek().type != tok_right)
			return Error::alloc(RPAREN, string("At ") +
					    s.location() +
					    ": Right paren expected.");
		s.next();
		return e;
	} 

	return Error::alloc(SYN, string("At ") + 
			    s.location() + ": Input syntax error");
}

// A helper for read_sexpr() which reads the balance of a list, after
// the opening ( has been consumed.
Ptr<Evaluable> read_rest_list(Scanner &s)
{
	// If we've reached ), we're done.
	if(s.peek().type == tok_right || s.peek().type == tok_eof)
		return the_nil;

	// Read the next list member.
	Ptr<Evaluable> car = read_sexpr(s);
	if(car.points_to(typeid(Error))) return car;

	// Check for a dot indicating dot-pair notation.
	if(s.peek().type == tok_period) {
		// Since we have a dot, we must have a cdr value then a closing
		// ) to finish out the pair.  Read and enforce.
		s.next();
		Ptr<Evaluable> cdr = read_sexpr(s);
		if(cdr.points_to(typeid(Error))) return cdr;
		return Pair::alloc(car, cdr);
	} else {
		// Just more list.
		Ptr<Evaluable> cdr = read_rest_list(s);
		if(cdr.points_to(typeid(Error))) return cdr;
		return Pair::alloc(car, cdr);
	}
}

// This is the include file search path.  It is initialized in main()
// from the parameters, then the environment, then hardwired default.
vector<string> srch_list;

// Load a file.  Return the last value produced, or an error if the file
// could not be opened.
Ptr<Evaluable> load(string name, Ptr<Context> c)
{
	ifstream in;
	bool found = false;

	if(isabs(name)) {
		in.open(name.c_str(), ios::in);
		found = in;
	}
	else {
		// Search and try all the opens.
		vector<string>::iterator i;
		for(i = srch_list.begin(); i < srch_list.end(); ++i) {
			string fn = *i + name;
			in.open(fn.c_str(), ios::in);
			if(in) {
				found = true;
				break;
			}
			in.clear();
		}
	}

	// If it didn't, boom.
	if(!found)
		return Error::alloc(OPFAIL, string("Open of ") +
				    name + " failed.");

	// Load the contents.
	Scanner fs(in, name);
	Ptr<Evaluable> e = the_nil;
	while(fs.peek().type != tok_eof) {
		e = read_sexpr(fs);
		e = e->eval(c);
		if(e.points_to(typeid(Error))) return e;
	}
	return e;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//***  Section VII.  Main
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

// This is a utility used to separate a search path string into its 
// parts and add them to the global search path, srch_list.  Empty
// components are discarded.
void setsearch(string str)
{
	// Clear it out.
	srch_list.clear();

	// Go through the :-separated parts.
	int loc;
	while((loc = str.find(SEP_CHAR)) >= 0) {
		if(loc > 0)
			srch_list.push_back(str.substr(0, loc) + PATH_CHR);
		str = str.substr(loc+1);
	}
	if(str.length())
		srch_list.push_back(str + PATH_CHR);
}

// Load the file, print an message and exit if the load fails.  Used for
// loading the init file and files listed on the command line.
void bload(char *name, Ptr<Context> c)
{
	Ptr<Evaluable> res = load(name, c);
	if(res.points_to(typeid(Error))) {
		Error::history_off();
		cerr << "Error loading " << name << ": ";
		res->print(cerr);
		cerr << endl;
		exit(1);
	}
}

int main(int argc, char **argv) 
{
	// Search path environmet or default.
	char *src;
	if(getenv("LSP_PATH")) src = getenv("LSP_PATH");
	else src = LSP_PATH;

	// Bare mode (parameter).  Skips init load.
	bool bare = false;

	// Do not enter the interactive read-eval-print loop.
	bool batch = false;

	// Options.  The -s can replace search path.
	extern char *optarg;
	int found;
	while((found = getopt(argc, argv, "s:bqBle:S:F:C")) >= 0) {
		switch(found) {
		case 's':
			src = optarg;
			break;
#ifdef MEM_DEBUG
		case 'l':
			RefCtObj::reporting(atoi(optarg));
			break;
#endif
		case 'b':
			bare = true;
			break;
		case 'B':
			batch = true;
			break;
		case 'q':
			Error::history_off();
			break;
		case 'e': {
			int cnt = atoi(optarg);
			int bot = 2*cnt / 3;
			Error::toplim(cnt - bot);
			Error::botlim(bot); }
			break; 
		case 'S':
			Error::toplim(atoi(optarg));
			break;
		case 'F':
			Error::botlim(atoi(optarg));
			break;
		case 'C':
			cout << "Tom's Lisp " << VERSION << " compiled for "
			     << COMP_FOR << " systems." << endl;
			cout << "Compiled on " 
			     << __DATE__ << " " << __TIME__ << endl;
#ifdef __VERSION__
			cout << "Compiler version: " << __VERSION__ << endl;
#endif
			cout << "SEP_CHAR = \"" << SEP_CHAR << "\"" << endl;
			cout << "PATH_CHR = \"" << PATH_CHR << "\"" << endl;
			cout << "LSP_PATH = \"" << LSP_PATH << "\"" << endl;
			cout << "search path = \"" << src << "\"" << endl;
#ifdef MEM_DEBUG
			cout << "Memory debug on." << endl;
#endif
			break;
		default:
			exit(3);
		}
	}

	setsearch(src);

	// Create the main context.  This declares the main context object.
	Ptr<Context> maincon = Context::alloc();
#ifdef MEM_DEBUG
	RefCtObj::rpt("Main context created");
#endif

	// This section creates entries in the main context for the standard
	// definitions.  Most of these are Builtin functions, but there are 
	// also nil and #t, and Catch is down there a ways.
	maincon->set("nil", the_nil);
	maincon->set("#t", the_true);
	maincon->set("car", Builtin::alloc(bi_car, "car", "p"));
	maincon->set("cdr", Builtin::alloc(bi_cdr, "cdr", "p"));
	maincon->set("cons", Builtin::alloc(bi_cons, "cons", ".."));
	maincon->set("null?", Builtin::alloc(bi_null, "null?", "."));
	maincon->set("pair?", Builtin::alloc(bi_ispair, "pair?", "."));
	maincon->set("id?", Builtin::alloc(bi_isid, "id?", "."));
	maincon->set("int?", Builtin::alloc(bi_isint, "int?", "."));
	maincon->set("str?", Builtin::alloc(bi_isstring, "str?", "."));
	maincon->set("lambda?", Builtin::alloc(bi_islambda, "lambda?", "."));
	maincon->set("macro?", Builtin::alloc(bi_ismacro, "macro?", "."));
	maincon->set("builtin?", 
		     Builtin::alloc(bi_isbuiltin, "builtin?", "."));

	maincon->set("quote", Builtin::alloc(bi_quote, "quote", ".", false));
	maincon->set("eval", Builtin::alloc(bi_eval, "eval", "."));

	maincon->set("begin", Builtin::alloc(bi_begin, "begin", "+", false));
	maincon->set("cond", Builtin::alloc(bi_cond, "cond", "+", false));
	maincon->set("or", Builtin::alloc(bi_or, "or", "+", false));
	maincon->set("error", Builtin::alloc(bi_error, "error", "is"));
	maincon->set("catch", Catch::alloc());

	maincon->set("set", Builtin::alloc(bi_set, "set", "n."));
	maincon->set("scope", Builtin::alloc(bi_scope, "scope", "+", false));

	maincon->set("macro", Builtin::alloc(bi_macro, "macro", "F.", false));
	maincon->set("lambda", Builtin::alloc(bi_lambda, "lambda", 
					      "F.", false));

	maincon->set("exit", Builtin::alloc(bi_exit, "exit", "i"));

	maincon->set("+", Builtin::alloc(bi_plus, "+", "+i"));
	maincon->set("*", Builtin::alloc(bi_times, "*", "+i"));
	maincon->set("-", Builtin::alloc(bi_minus, "-", "ii"));
	maincon->set("/", Builtin::alloc(bi_div, "/", "ii"));
	maincon->set("%", Builtin::alloc(bi_mod, "%", "ii"));

	maincon->set("<", Builtin::alloc(bi_lt, "<", "ii"));
	maincon->set(">", Builtin::alloc(bi_gt, ">", "ii"));
	maincon->set("<=", Builtin::alloc(bi_le, "<=", "ii"));
	maincon->set(">=", Builtin::alloc(bi_ge, ">=", "ii"));
	maincon->set("=", Builtin::alloc(bi_eq, "=", "ii"));
	maincon->set("!=", Builtin::alloc(bi_ne, "!=", "ii"));
	maincon->set("eq?", Builtin::alloc(bi_ptreq, "eq?", ".."));
	maincon->set("equal?", Builtin::alloc(bi_equal, "equal?", ".."));

	maincon->set("strlen", Builtin::alloc(bi_strlen, "strlen", "s"));
	maincon->set("shatter", Builtin::alloc(bi_shatter, "shatter", "s"));
	maincon->set("collect", Builtin::alloc(bi_collect, "collect", "(+s)"));
	maincon->set("chr", Builtin::alloc(bi_chr, "chr", "i"));
	maincon->set("ord", Builtin::alloc(bi_ord, "ord", "s"));

	maincon->set("load", Builtin::alloc(bi_load, "load", "s"));
	maincon->set("print", Builtin::alloc(bi_print, "print", "."));
	maincon->set("sprint", Builtin::alloc(bi_sprint, "sprint", "s"));

#ifdef MEM_DEBUG
	maincon->set("memrpt", Builtin::alloc(bi_memrpt, "memrpt", ""));
	maincon->set("aloc-log", Builtin::alloc(bi_aloc_log, "aloc-log", "i"));
#endif

	// Create definitions for the standard error classes.
	maincon->set("ERR_UNDEF", Int::alloc(UNDEF));
	maincon->set("ERR_BADOP", Int::alloc(BADOP));
	maincon->set("ERR_BADARG", Int::alloc(BADARG));
	maincon->set("ERR_SHORTARG", Int::alloc(SHORTARG));
	maincon->set("ERR_BADTYPE", Int::alloc(BADTYPE));
	maincon->set("ERR_LONGARG", Int::alloc(LONGARG));
	maincon->set("ERR_RPAREN", Int::alloc(RPAREN));
	maincon->set("ERR_SYN", Int::alloc(SYN));
	maincon->set("ERR_OPFAIL", Int::alloc(OPFAIL));
	maincon->set("ERR_DIVZERO", Int::alloc(DIVZERO));
	maincon->set("ERR_INTER", Int::alloc(INTER));
	maincon->set("NEXT_ERR", Int::alloc(enct));

	// A couple of important environment variables.
	if(getenv("USER"))
		maincon->set("USER", Str::alloc(getenv("USER")));
	if(getenv("HOME"))
		maincon->set("HOME", Str::alloc(getenv("HOME")));

	// What the heck.  Might be useful.
	maincon->set("PID", Int::alloc(getpid()));

	// Load the standard init file.
	if(!bare) bload(INIT_FILE, maincon);

	// Load the command line files.
	for(argc -= optind, argv += optind; argc--; ++argv) 
		bload(*argv, maincon);

	// For batch mode, we're done.
	if(batch) exit(0);

	// Main read and print loop.  This reads s-expressions from the
	// console (standard input), evaluates each, and prints the result.
	initsig();					// Init sig handling.
	Scanner in(cin, "[standard input]", &cout);	// Init tokenizer.
	cout << "Welcome to Tom's Lisp " << VERSION <<  // Welcome msg.
		".  Recur well." << endl;
#ifdef MEM_DEBUG
	RefCtObj::rpt("Before main loop");
#endif
	// Read-eval-print loop.
	while(in.peek().type != tok_eof) {
		// Read.
		Ptr<Evaluable> e = read_sexpr(in);

		// Reset ^c flag, then evaluate the expression.
		signalled = false;
		e = e->eval(maincon);

		// Print the result.
		e->print(cout);
		cout << endl;

		// Clear the input stream.
		in.clear();
	}
	cout << endl;

#ifdef MEM_DEBUG
	// This actually does a bit extra cleanup that we don't bother with
	// when we're exiting anyway.  But it tests the algorithms.
	RefCtObj::rpt("Main loop exit");
	maincon->last_rites();
	maincon.nullify();
	RefCtObj::rpt("Finish");
#endif
}

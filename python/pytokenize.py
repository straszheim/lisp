#!/usr/bin/python

from tokenize import *
import token
from StringIO import StringIO
import sys

def decistmt(s):
    """Substitute Decimals for floats in a string of statements.

    >>> from decimal import Decimal
    >>> s = 'print +21.3e-5*-.1234/81.7'
    >>> decistmt(s)
    "print +Decimal ('21.3e-5')*-Decimal ('.1234')/Decimal ('81.7')"

    >>> exec(s)
    -3.21716034272e-007
    >>> exec(decistmt(s))
    -3.217160342717258261933904529E-7

    """
    result = []
    g = generate_tokens(StringIO(s).readline)   # tokenize the string
    for toknum, tokval, rr1, rr2, rr3  in g:
        print token.tok_name[toknum], tokval, " -- ", rr1, rr2, rr3
        if toknum == NUMBER and '.' in tokval:  # replace NUMBER tokens
            result.extend([
                (NAME, 'Decimal'),
                (OP, '('),
                (STRING, repr(tokval)),
                (OP, ')')
            ])
        else:
            result.append((toknum, tokval))
    return untokenize(result)


ifile = file(sys.argv[1])


print decistmt(''.join(ifile.readlines()))


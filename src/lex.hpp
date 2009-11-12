#ifndef LISP_LEX_HPP_INCLUDED
#define LISP_LEX_HPP_INCLUDED

namespace lisp 
{
  enum token_ids
    {
      OPENPAREN,
      CLOSEPAREN,
      QUOTE,
      BACKQUOTE,
      COMMA,
      COMMA_AT,
      IDENTIFIER,
      STRING,
      COMMENT
    };
}

#endif

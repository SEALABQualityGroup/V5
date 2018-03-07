/*
 *  Parser.cpp
 *  ModLang
 *
 *  Created by Martin Mroz on 14/01/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "NonCopyable.h"
#include "Parser.h"
#include "Scanner.h"
#include <sstream>
#include <cstdio>
#include <cstdlib>

/* Parser Interface (Automatically generated by Lemon) */
extern void *ModLangParserAlloc(void *(*mallocProc)(size_t));
extern void ModLangParserFree(void *p, void (*freeProc)(void*));
extern void ModLangParser(void *yyp, int yymajor, LQX::ScannerToken* yyminor, LQX::Parser* ctx);
extern const char *ModLangParserTokenName(int tokenType);
extern void ModLangParserTrace(FILE *TraceFILE, const char *zTracePrompt);

#pragma mark -

namespace LQX {

  /* This array must match PT_P_* in Parser_pre.h and Parser_pre.ypp */
  static const char* tokenNameMap[] = {
    "End Of Stream",
    "Boolean Value",
    "String Value",
    "Numerical Value",
    "Identifier",
    "for",
    "foreach",
    "in",
    "while",
    "break",
    "if",
    "else",
    "return",
    "write",
    "read",
    "append",
    "read_loop",
    "file_open",
    "print",
    "println",
    "print_spaced",
    "println_spaced",
    "file_close",
    "read_data",
    "(",
    ")",
    "{",
    "}",
    "[",
    "]",
    ".",
    "**",
    "*",
    "/",
    "+",
    "-",
    "%",
    "<<",
    ">>",
    "||",
    "&&",
    "~",
    "==",
    "!=",
    "<",
    ">",
    "<=",
    ">=",
    "=",
    "**=",
    "*=",
    "/=",
    "+=",
    "-=",
    "%=",
    "<<=",
    ">>=",
    ";",
    ",",
    "->",
    "NULL",
    "f()",
    "...",
    "<invalid>"
  };
  
  const char* ParserTokenToName(ParserToken token)
  {
    /* Make sure the token is within range */
    if (token < PT_EOS || token > PT_ERROR) {
      token = PT_ERROR;
    }
    
    /* Return the name from the map itself */
    return tokenNameMap[(unsigned)token];
  }
  
  /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  
  Parser::Parser() : _syntaxErrorLineno(0), _syntaxErrorDidOccur(false), 
    _syntaxErrorMessage(""), _rootStatementList(NULL), _parser(NULL)
  {
    /* Allocate the parser with system malloc() */
    _parser = ModLangParserAlloc(malloc);
  }
  
  Parser::~Parser() 
  {
    /* Release the parser with system free() */
    ModLangParserFree(_parser, free);
  }
  
  Parser::Parser(const Parser&)
  {
    throw NonCopyableException();
  }
  
  Parser& Parser::operator=(const Parser&)
  {
    throw NonCopyableException();
  }
  
  bool Parser::processToken(LQX::ScannerToken* value)
  {
    /* Parse the given token along side the code and Parser */
    if (!value) value = new LQX::ScannerToken(0, LQX::PT_EOS);
    ModLangParser(_parser, value->getTokenCode(), value, this);
    return !_syntaxErrorDidOccur;
  }
  
  unsigned Parser::getSyntaxErrorLineNumber() const
  {
    /* Returns the last syntax error line */
    return _syntaxErrorLineno;
  }
  
  const std::string& Parser::getLastSyntaxErrorMessage() const
  {
    /* Returns the last syntax error data */
    return _syntaxErrorMessage;
  }
  
  bool Parser::getSyntaxErrorDidOccur() const
  {
    /* Returns whether or not an error occurred */
    return _syntaxErrorDidOccur;
  }
  
  void Parser::clearSyntaxError()
  {
    /* Go in and clear out any error flags */
    _syntaxErrorDidOccur = false;
    _syntaxErrorMessage = "";
    _syntaxErrorLineno = 0;
  }
  
  void Parser::parseFailed() 
  {
  }
  
  void Parser::syntaxError(LQX::ScannerToken* problemToken)
  {
    /* Mark down that an error occurred */
    _syntaxErrorDidOccur = true;
    _syntaxErrorMessage = "error: Unexpected token `";
    _syntaxErrorMessage += ParserTokenToName(problemToken->getTokenCode());
    _syntaxErrorMessage += "'";
    _syntaxErrorLineno = problemToken->getLineNumber();
  }
  
  void Parser::parseAccepted() 
  {
  }
  
  void Parser::setRootStatementList(std::vector<SyntaxTreeNode*>* rootList)
  {
    /* Set the new root statement list */
    _rootStatementList = rootList;
  }
  
  std::vector<SyntaxTreeNode*>* Parser::getRootStatementList()
  {
    /* Return the new root statement list */
    return _rootStatementList;
  }
  
}

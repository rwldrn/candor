#ifndef _SRC_PARSER_H_
#define _SRC_PARSER_H_

#include "lexer.h"
#include "ast.h"

#include <assert.h> // assert
#include <stdlib.h> // NULL

namespace dotlang {

class Parser : public Lexer {
 public:
  Parser(const char* source, uint32_t length) : Lexer(source, length) {
    ast_ = new BlockStmt(NULL);
  }


  class Position {
   public:
    Position(Lexer* lexer) : lexer_(lexer), committed_(false) {
      if (lexer_->queue()->length == 0) {
        offset_ = lexer_->offset_;
      } else {
        offset_ = lexer_->queue()->Head()->value->offset_;
      }
    }

    ~Position() {
      if (!committed_) {
        while (lexer_->queue()->length > 0) delete lexer_->queue()->Shift();
        lexer_->offset_ = offset_;
      }
    }

    inline AstNode* Commit(AstNode* result) {
      if (result != NULL) {
        committed_ = true;
      }
      return result;
    }

   private:
    Lexer* lexer_;
    uint32_t offset_;
    bool committed_;
  };


  inline AstNode* Wrap(AstNode::Type type, AstNode* original) {
    AstNode* wrap = new AstNode(type);
    wrap->children()->Push(original);
    return wrap;
  }


  inline AstNode* ast() {
    return ast_;
  }


  AstNode* Execute();
  AstNode* ParseStatement();
  AstNode* ParseExpression();
  AstNode* ParsePrefixUnop(AstNode::Type type);
  AstNode* ParseBinOp(TokenType type, AstNode* lhs);
  AstNode* ParsePrimary();
  AstNode* ParseMember();
  AstNode* ParseBlock();
  AstNode* ParseScope();

  AstNode* ast_;
};

} // namespace dotlang

#endif // _SRC_PARSER_H_
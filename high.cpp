#include <QtGui>

#include "high.h"

CommentHighlighter::CommentHighlighter(QTextDocument *parent)
     : QSyntaxHighlighter(parent)
{
HighlightingRule rule;

//  The escape sequence \s should denote a space and it says so in the docs
//  but if you use it you get a 'unrecognised escape char' warning so use \x20 instead

    M_WordFormat.setForeground(Qt::red);
    M_WordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("[mMtT][^\x20]*");
    rule.format = M_WordFormat;
    highlightingRules.append(rule);

    G_WordFormat.setForeground(Qt::darkGreen);
    G_WordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("[gG][^\x20]*");
    rule.format = G_WordFormat;
    highlightingRules.append(rule);

    F_WordFormat.setForeground(Qt::darkYellow);
    F_WordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("[fF][^\x20 ]*");
    rule.format = F_WordFormat;
    highlightingRules.append(rule);

    S_WordFormat.setForeground(Qt::magenta);
    S_WordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("[sS][^\x20 ]*");
    rule.format = S_WordFormat;
    highlightingRules.append(rule);

    PQ_WordFormat.setForeground(Qt::green);
    PQ_WordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("[pPqQ][^\x20 ]*");
    rule.format = PQ_WordFormat;
    highlightingRules.append(rule);
    
    XYZ_WordFormat.setForeground(Qt::black);
    XYZ_WordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("[xXyYzZaAbBcCuUvVwW][^\x20 ]*");
    rule.format = XYZ_WordFormat;
    highlightingRules.append(rule);
    
    IJKR_WordFormat.setForeground(Qt::darkGray);
    IJKR_WordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("[iIjJkKrR][^\x20 ]*");
    rule.format = IJKR_WordFormat;
    highlightingRules.append(rule);
    
    Param_WordFormat.setForeground(Qt::darkCyan);
    Param_WordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("#[^\x20 ]*");
    rule.format = Param_WordFormat;
    highlightingRules.append(rule);    

// do comments last then won't get colouring of text containing M G F S T etc'
    semicolonCommentFormat.setForeground(Qt::blue);
    semicolonCommentFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp(";[^\n]*");
    rule.format = semicolonCommentFormat;
    highlightingRules.append(rule);

    braceCommentFormat.setForeground(Qt::blue);
    braceCommentFormat.setFontWeight(QFont::Bold);    
    rule.pattern = QRegExp("\\([^\n]*");
    rule.format = braceCommentFormat;
    highlightingRules.append(rule);
    


}

void CommentHighlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) 
        {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) 
            {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
            }
        }
    setCurrentBlockState(0);

}


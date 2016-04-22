#ifndef COMMENT_HIGHLIGHTER_H
#define COMMENT_HIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>


class CommentHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    CommentHighlighter(QTextDocument *parent = 0);
protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
       {
       QRegExp pattern;
       QTextCharFormat format;
       };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat semicolonCommentFormat;
    QTextCharFormat braceCommentFormat;
    QTextCharFormat M_WordFormat;
    QTextCharFormat G_WordFormat;
    QTextCharFormat F_WordFormat;
    QTextCharFormat S_WordFormat;
    QTextCharFormat PQ_WordFormat;
    QTextCharFormat XYZ_WordFormat;
    QTextCharFormat IJKR_WordFormat;
    QTextCharFormat Param_WordFormat;            
};

#endif

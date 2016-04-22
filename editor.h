#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QObject>
#include <QtDebug>
#include <QAction>
#include <QMenu>

#include "high.h"

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

class LineNumberArea;


class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = 0);

    QString getCurrentText();
    QString formatLine(QString);
    
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    int getLineNo();
    int getLineCount();
    void cursorUp();
    void cursorDown();
    void highlightLine(int);
    QAction *actionRunFrom;
    
    bool isModified();
    void appendNewPlainText(const QString &);
    void clear();
    QStringList contents;
 
Q_SIGNALS:
    void runFromSelected(int line);
    
protected:
    void resizeEvent(QResizeEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

    
private slots:
    virtual void updateLineNumberAreaWidth(int newBlockCount);
    virtual void highlightCurrentLine();
    virtual void updateLineNumberArea(const QRect &, int);
    virtual void onRunFrom();
     
private:
    QWidget *lineNumberArea;
    CommentHighlighter *hiLite;
};


 class LineNumberArea : public QWidget
 {
 public:
     LineNumberArea(CodeEditor *editor) : QWidget(editor) {
         codeEditor = editor;
     }

     QSize sizeHint() const {
         return QSize(codeEditor->lineNumberAreaWidth(), 0);
     }

 protected:
     void paintEvent(QPaintEvent *event) {
         codeEditor->lineNumberAreaPaintEvent(event);
     }

 private:
     CodeEditor *codeEditor;
 };


 #endif

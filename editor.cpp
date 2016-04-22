#include <QtGui>
#include <QtDebug>
#include <QApplication>
#include "editor.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// this class is used within Designer promoted from QPlainTextEdit so that it can be inserted directly and not into another widget
//      New functions:
// cursorUp()
// cursorDown()
// getLineNo()
// highlightLine()
// contextMenuEvent()
// onRunFrom()
// runFromSelected()
// isModified()
//      Overloaded:
// appendNewPlainText(const QString &text)
// clear()
//
// Did put file loading mechanism into code editor but caused problems with emc not loading file 
// reason for this unknown, but isolated to that one issue
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    actionRunFrom = new QAction(this);
    actionRunFrom->setObjectName(QString::fromUtf8("actionRunFrom"));
    actionRunFrom->setText("Run From");

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    
    hiLite = new CommentHighlighter(document());
    
    highlightCurrentLine();
}

// formats first and adds to QStringList before appending 
// for later comparison using list to test if text changed

void CodeEditor::appendNewPlainText(const QString &text)
{
QString str;

    //format the text to space entries if necessary
    str = formatLine(text);
    contents << str;
    QPlainTextEdit::appendPlainText(str);
}

// overloaded to clear the QStringList first

void CodeEditor::clear()
{
    contents.clear();
    QPlainTextEdit::clear();

}

QString CodeEditor::formatLine(QString text)
{
QString str, str2;
QStringList list;
    // get rid of extra spaces, convert to UC and make 2 copies
    str = text;
    str = str.simplified();
    str2 = str = str.toUpper();

    // deal with comments in str2    
    // if starts with ( or ; bypass altogether)
    if(str2.startsWith('(') || str2.startsWith(';') )
            return str2;
    
    if(str2.contains('(') )
        {
        list = str2.split("(");
        str = list[0];
        str2 = " (" + list[1];
        }
    else if(str2.contains(';') )
        {
        list = str2.split(";", QString::SkipEmptyParts); // skip because could have 2 or more ;
        str = list[0];
        str2 = " ;" + list[1];
        }
    else
        str2 = "";
    // now process str, which either contains whole string
    // or one before comments
        
    // get rid of line numbers        
    str.remove(QRegExp("N([0-9]*)"));
    // space G Code elements for readability
    str.replace(QRegExp("G([0-9]*)"), " G\\1");
    str.replace(QRegExp("M([0-9]*)"), " M\\1");
    str.replace(QRegExp("F([0-9]*)"), " F\\1");
    str.replace(QRegExp("S([0-9]*)"), " S\\1");
    str.replace(QRegExp("P([0-9]*)"), " P\\1");
    str.replace(QRegExp("Q([0-9]*)"), " Q\\1");
    str.replace(QRegExp("X([0-9]*)"), " X\\1");
    str.replace(QRegExp("Y([0-9]*)"), " Y\\1");
    str.replace(QRegExp("Z([0-9]*)"), " Z\\1");
    str.replace(QRegExp("I([0-9]*)"), " I\\1");
    str.replace(QRegExp("J([0-9]*)"), " J\\1");
    str.replace(QRegExp("K([0-9]*)"), " K\\1");
    str.replace(QRegExp("R([0-9]*)"), " R\\1");
    // get rid of any double spacing and leading spaces
    str.replace(QRegExp("  "), " ");
    str = str.simplified();
    // push to 2nd line if stupidly been put on same line   
    if(str2.length())
        str = str + "\n" + str2;
    
    return str;
}

/////////////////////////////////////////////////////////////////////////////////////

// big problem with the base editor was that it registers that document changed and that
// modification changed (usually by using Ctrl Z or Ctrl Shift Z)
// but you still could not tell if the document is now different overall or not
// By saving a copy of what was loaded and then comparing it to what is present, it
// reports accurately on any change.

bool CodeEditor::isModified()
{
QString txt = toPlainText();
QStringList list = txt.split( "\n");

    if( contents.size() != list.size() )
        return true;
        
    for(int x = 0; x < contents.size(); x++)
        {
        if( contents[x] != list[x] )
            return true;
        }
    return false;
}

QString CodeEditor::getCurrentText()
{
QTextDocument *doc = document();

    QTextBlock block = doc->findBlock( textCursor().position());
    return(block.text().trimmed().toLatin1());
}    


//////////////////////////////////////////////////////////////////////////////

void CodeEditor::contextMenuEvent(QContextMenuEvent *event)
{
QMenu *menu = createStandardContextMenu();
    menu->addSeparator();
    menu->addAction(actionRunFrom);
    connect(actionRunFrom, SIGNAL(triggered()), this, SLOT(onRunFrom()));
    menu->exec(event->globalPos());
}

void CodeEditor::onRunFrom()
{
    emit runFromSelected(getLineNo());
}

//////////////////////////////////////////////////////////////////////////////

void CodeEditor::cursorUp()
{
    moveCursor(QTextCursor::PreviousBlock);
}

void CodeEditor::cursorDown()
{
    moveCursor(QTextCursor::NextBlock);
}

int CodeEditor::getLineNo()
{
int numBlocks = blockCount();
QTextDocument *doc = document();

    QTextBlock blk = doc->findBlock( textCursor().position() );
    QTextBlock blk2 = doc->begin();
    
    for(int x = 1; x < numBlocks; x++)
        {
        if(blk == blk2)
            return x;
        blk2 = blk2.next();
        }
    return 0;
}


void CodeEditor::highlightLine(int line)
{
int num = 0;
    // when file loaded, highlights first blank line at end with EOF, 
    // so never matched and returns 0 unless go up 1 first
    if( blockCount())
        {
        if(line > 0 && line <= blockCount())
            {
            cursorUp();
            num = getLineNo();
            if(num > line)
                {
                do  {
                    cursorUp();
                    num--;
                    }while(num > line);
                }
            else
                {
                while(num < line)
                    {
                    cursorDown();
                    num++;
                    }
                }          
            }
        else
            qDebug() << "Invalid line number passed";
        }
    else
        qDebug() << "No blocks found";
}

int CodeEditor::getLineCount() { return blockCount() - 1;}


///////////////////////////////////////////////////////////////////////////////////


 int CodeEditor::lineNumberAreaWidth()
 {
     int digits = 1;
     int max = qMax(1, blockCount());
     while (max >= 10) {
         max /= 10;
         ++digits;
     }

     int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

     return space;
 }



 void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
 {
     setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
 }



 void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
 {
     if (dy)
         lineNumberArea->scroll(0, dy);
     else
         lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

     if (rect.contains(viewport()->rect()))
         updateLineNumberAreaWidth(0);
 }



 void CodeEditor::resizeEvent(QResizeEvent *e)
 {
     QPlainTextEdit::resizeEvent(e);

     QRect cr = contentsRect();
     lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
 }




 void CodeEditor::highlightCurrentLine()
 {
     QList<QTextEdit::ExtraSelection> extraSelections;

     QTextEdit::ExtraSelection selection;

     QColor lineColor = QColor(Qt::yellow).lighter(160);

     selection.format.setBackground(lineColor);
     selection.format.setProperty(QTextFormat::FullWidthSelection, true);
     selection.cursor = textCursor();
     selection.cursor.clearSelection();
     extraSelections.append(selection);
    
     setExtraSelections(extraSelections);
 }



 void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
 {
     QPainter painter(lineNumberArea);
     painter.fillRect(event->rect(), Qt::lightGray);


     QTextBlock block = firstVisibleBlock();
     int blockNumber = block.blockNumber();
     int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
     int bottom = top + (int) blockBoundingRect(block).height();

     while (block.isValid() && top <= event->rect().bottom()) {
         if (block.isVisible() && bottom >= event->rect().top()) {
             QString number = QString::number(blockNumber + 1);
             painter.setPen(Qt::black);
             painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                              Qt::AlignRight, number);
         }

         block = block.next();
         top = bottom;
         bottom = top + (int) blockBoundingRect(block).height();
         ++blockNumber;
     }
 }

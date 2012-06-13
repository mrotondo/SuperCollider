/*
    SuperCollider Qt IDE
    Copyright (c) 2012 Jakob Leben & Tim Blechmann
    http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <cassert>
#include <algorithm>

#include "highlighter.hpp"
#include "../../core/main.hpp"
#include "../../core/settings/manager.hpp"

#include <QApplication>

namespace ScIDE {

SyntaxHighlighterGlobals * SyntaxHighlighterGlobals::mInstance = 0;

SyntaxHighlighterGlobals::SyntaxHighlighterGlobals( Main *main ):
    QObject(main)
{
    Q_ASSERT(mInstance == 0);
    mInstance = this;

    initSyntaxRules();

    // initialize formats from settings:
    applySettings(main->settings());

    connect(main, SIGNAL(applySettingsRequest(Settings::Manager*)),
            this, SLOT(applySettings(Settings::Manager*)));
}

void SyntaxHighlighterGlobals::initSyntaxRules()
{
    /* NOTE:

    The highlighting algorithm demands that all regexps
    start with a caret "^", to only match at beginning of string.

    Order is important:
    -- floatRegexp is subset of radixFloatRegex -> must come later
    -- classRegexp and primitiveRegexp are subsets of symbolArgRegexp -> must come later

    */

    mInCodeRules << SyntaxRule( SyntaxRule::WhiteSpace, "^\\s+" );

    initKeywords();
    initBuiltins();

    mInCodeRules << SyntaxRule( SyntaxRule::SymbolArg, "^\\b[A-Za-z_]\\w*\\:" );

    mInCodeRules << SyntaxRule( SyntaxRule::Identifier, "^[a-z]\\w*" );

    mInCodeRules << SyntaxRule( SyntaxRule::Class, "^\\b[A-Z]\\w*" );

    mInCodeRules << SyntaxRule( SyntaxRule::Primitive, "^\\b_\\w+" );

    mInCodeRules << SyntaxRule( SyntaxRule::Symbol, "^(\\\\\\w*|\\'([^\\'\\\\]*(\\\\.[^\\'\\\\]*)*)\\')" );

    mInCodeRules << SyntaxRule( SyntaxRule::Char, "^\\$\\\\?." );

    mInCodeRules << SyntaxRule( SyntaxRule::String, "^\"([^\"\\\\]*(\\\\.[^\"\\\\]*)*)\"" );

    mInCodeRules << SyntaxRule( SyntaxRule::EnvVar, "^~\\w+" );

    mInCodeRules << SyntaxRule( SyntaxRule::RadixFloat, "^\\b\\d+r[0-9a-zA-Z]*(\\.[0-9A-Z]*)?" );

    // do not include leading "-" in float highlighting, as there's no clear
    // rule whether it is not rather a binary operator
    mInCodeRules << SyntaxRule( SyntaxRule::Float, "^\\b((\\d+(\\.\\d+)?([eE][-+]?\\d+)?(pi)?)|pi)" );

    mInCodeRules << SyntaxRule( SyntaxRule::HexInt, "^\\b0(x|X)(\\d|[a-f]|[A-F])+" );

    mInCodeRules << SyntaxRule( SyntaxRule::SingleLineComment, "^//[^\r\n]*" );

    mInCodeRules << SyntaxRule( SyntaxRule::MultiLineCommentStart, "^/\\*" );

    mInSymbolRegexp.setPattern("([^\'\\\\]*(\\\\.[^\'\\\\]*)*)");
    mInStringRegexp.setPattern("([^\"\\\\]*(\\\\.[^\"\\\\]*)*)");
}

void SyntaxHighlighterGlobals::initKeywords()
{
    QStringList keywords;
    keywords << "arg"
             << "classvar"
             << "const"
             << "super"
             << "this"
             << "thisFunction"
             << "thisFunctionDef"
             << "thisMethod"
             << "thisProcess"
             << "thisThread"
             << "thisThread"
             << "var";

    QString keywordPattern = QString("^\\b(%1)\\b").arg(keywords.join("|"));
    mInCodeRules << SyntaxRule(SyntaxRule::Keyword, keywordPattern);
}

void SyntaxHighlighterGlobals::initBuiltins()
{
    QStringList builtins;
    builtins << "false"
             << "inf"
             << "nil"
             << "true";

    QString builtinsPattern = QString("^\\b(%1)\\b").arg(builtins.join("|"));
    mInCodeRules << SyntaxRule(SyntaxRule::Builtin, builtinsPattern);
}

void SyntaxHighlighterGlobals::applySettings( Settings::Manager *s )
{
    QString key("IDE/editor/highlighting");
    applySettings( s, key + "/normal", PlainFormat );
    applySettings( s, key + "/keyword", KeywordFormat );
    applySettings( s, key + "/builtin", BuiltinFormat );
    applySettings( s, key + "/primitive", PrimitiveFormat );
    applySettings( s, key + "/class", ClassFormat );
    applySettings( s, key + "/number", NumberFormat );
    applySettings( s, key + "/symbol", SymbolFormat );
    applySettings( s, key + "/envvar", EnvVarFormat );
    applySettings( s, key + "/string", StringFormat );
    applySettings( s, key + "/char", CharFormat );
    applySettings( s, key + "/comment", CommentFormat );

    Q_EMIT(syntaxFormatsChanged());
}

void SyntaxHighlighterGlobals::applySettings( Settings::Manager *s, const QString &key, SyntaxFormat type )
{
    mFormats[type] = s->value(key).value<QTextCharFormat>();
}

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent):
    QSyntaxHighlighter( parent )
{
    mGlobals = SyntaxHighlighterGlobals::instance();

    connect(mGlobals, SIGNAL(syntaxFormatsChanged()), this, SLOT(rehighlight()));
}

SyntaxRule::Type SyntaxHighlighter::findMatchingRule (const QString& text, int& currentIndex, int& lengthOfMatch)
{
    int matchLength = -1;
    SyntaxRule::Type matchRule = SyntaxRule::None;

    QVector<SyntaxRule>::const_iterator it  = mGlobals->mInCodeRules.constBegin();
    QVector<SyntaxRule>::const_iterator end = mGlobals->mInCodeRules.constEnd();

    for (; it != end; ++it) {
        SyntaxRule const & rule = *it;
        int matchIndex = rule.expr.indexIn(text, currentIndex, QRegExp::CaretAtOffset);
        // a guard to ensure all regexps match only at beginning of string:
        assert(matchIndex <= currentIndex);
        if (matchIndex != -1) {
            matchRule = rule.type;
            matchLength = rule.expr.matchedLength();
            break;
        }
    }

    lengthOfMatch = matchRule == SyntaxRule::None ? 0 : matchLength;
    return matchRule;
}

void SyntaxHighlighter::highlightBlockInCode(const QString& text, int & currentIndex, int & currentState)
{
    TextBlockData *blockData = static_cast<TextBlockData*>(currentBlockUserData());
    Q_ASSERT(blockData);

    const QTextCharFormat * formats = mGlobals->formats();

    do {
        static QString brackets("(){}[]");
        static QChar stringStart('\"');
        static QChar symbolStart('\'');

        QChar currentChar = text[currentIndex];

        if (currentChar == stringStart) {
            currentState = inString;
            setFormat(currentIndex, 1, formats[StringFormat]);
            currentIndex += 1;
            return;
        }

        if (currentChar == symbolStart) {
            currentState = inSymbol;
            setFormat(currentIndex, 1, formats[SymbolFormat]);
            currentIndex += 1;
            return;
        }

        if (brackets.contains(currentChar)) {
            blockData->brackets.push_back( BracketInfo(currentChar.toAscii(), currentIndex) );
            ++currentIndex;
            continue;
        }

        int lenghtOfMatch;
        SyntaxRule::Type rule = findMatchingRule(text, currentIndex, lenghtOfMatch);

        switch (rule)
        {
        case SyntaxRule::Class:
            setFormat(currentIndex, lenghtOfMatch, formats[ClassFormat]);
            break;

        case SyntaxRule::Builtin:
            setFormat(currentIndex, lenghtOfMatch, formats[BuiltinFormat]);
            break;

        case SyntaxRule::Primitive:
            setFormat(currentIndex, lenghtOfMatch, formats[PrimitiveFormat]);
            break;

        case SyntaxRule::Keyword:
            setFormat(currentIndex, lenghtOfMatch, formats[KeywordFormat]);
            break;

        case SyntaxRule::Symbol:
            setFormat(currentIndex, lenghtOfMatch, formats[SymbolFormat]);
            break;

        case SyntaxRule::SymbolArg:
            // Omit the trailing ":" that was included in the regexp:
            setFormat(currentIndex, lenghtOfMatch-1, formats[SymbolFormat]);
            break;

        case SyntaxRule::EnvVar:
            setFormat(currentIndex, lenghtOfMatch, formats[EnvVarFormat]);
            break;

        case SyntaxRule::String:
            setFormat(currentIndex, lenghtOfMatch, formats[StringFormat]);
            break;

        case SyntaxRule::Char:
            setFormat(currentIndex, lenghtOfMatch, formats[CharFormat]);
            break;

        case SyntaxRule::SingleLineComment:
            setFormat(currentIndex, lenghtOfMatch, formats[CommentFormat]);
            break;

        case SyntaxRule::Float:
        case SyntaxRule::HexInt:
        case SyntaxRule::RadixFloat:
            setFormat(currentIndex, lenghtOfMatch, formats[NumberFormat]);
            break;

        case SyntaxRule::MultiLineCommentStart:
            setFormat(currentIndex, lenghtOfMatch, formats[CommentFormat]);
            currentIndex += lenghtOfMatch;
            currentState = inComment;
            return;

        case SyntaxRule::None:
            currentIndex += 1;

        default:
            ;
        }

        currentIndex += lenghtOfMatch;
    } while (currentIndex < text.size());
}

void SyntaxHighlighter::highlightBlockInString(const QString& text, int& currentIndex, int& currentState)
{
    const QRegExp &expr = mGlobals->mInStringRegexp;
    int matchIndex = expr.indexIn(text, currentIndex);
    if (matchIndex == -1)
        assert(false);

    int matchLength = expr.matchedLength();
    setFormat(currentIndex, matchLength, mGlobals->format(StringFormat));
    currentIndex += matchLength;
    if (currentIndex == text.size()) {
        // end of block
        currentState = inString;
        return;
    }

    static const QChar endOfString('\"');
    if (text[currentIndex] == endOfString)
        currentState = inCode;

    setFormat(currentIndex, 1, mGlobals->format(StringFormat));
    ++currentIndex;
    return;
}

void SyntaxHighlighter::highlightBlockInSymbol(const QString& text, int& currentIndex, int& currentState)
{
    const QRegExp &expr = mGlobals->mInSymbolRegexp;
    int matchIndex = expr.indexIn(text, currentIndex);
    if (matchIndex == -1)
        assert(false);

    int matchLength = expr.matchedLength();
    setFormat(currentIndex, matchLength, mGlobals->format(SymbolFormat));
    currentIndex += matchLength;
    if (currentIndex == text.size()) {
        // end of block
        currentState = inSymbol;
        return;
    }

    static const QChar endOfSymbol('\'');
    if (text[currentIndex] == endOfSymbol)
        currentState = inCode;

    setFormat(currentIndex, 1, mGlobals->format(SymbolFormat));
    ++currentIndex;
    return;
}

void SyntaxHighlighter::highlightBlockInComment(const QString& text, int& currentIndex, int& currentState)
{
    int index = currentIndex;
    int maxIndex = text.size() - 1;

    static const QString commentStart("/*");
    static const QString commentEnd("*/");

    int commentStartIndex = -2;
    int commentEndIndex   = -2;

    while(index < maxIndex) {
        if ((commentStartIndex == -2) || (commentStartIndex < index))
            if (commentStartIndex != -1)
                commentStartIndex = text.indexOf(commentStart, index);

        if ((commentEndIndex == -2) || (commentEndIndex < index))
            if (commentEndIndex != -1)
                commentEndIndex   = text.indexOf(commentEnd, index);

        if (commentStartIndex == -1) {
            if (commentEndIndex == -1) {
                index = maxIndex;
            } else {
                index = commentEndIndex + 2;
                --currentState;
            }
        } else {
            if (commentEndIndex == -1) {
                index = commentStartIndex + 2;
                ++currentState;
            } else {
                if (commentStartIndex < commentEndIndex) {
                    index = commentStartIndex + 2;
                    ++currentState;
                } else {
                    index = commentEndIndex + 2;
                    --currentState;
                }
            }
        }
        if (currentState < inComment) {
            currentState = inCode;
            break;
        }
    }

    if(currentState == inCode) {
        setFormat(currentIndex, index - currentIndex, mGlobals->format(CommentFormat));
        currentIndex = index;
    }
    else {
        setFormat(currentIndex, text.size() - currentIndex, mGlobals->format(CommentFormat));
        currentIndex = text.size();
    }

    return;
}

void SyntaxHighlighter::highlightBlock(const QString& text)
{
    int currentIndex = 0;

    int currentState = previousBlockState();
    if (currentState == -1)
        currentState = 0;

    TextBlockData *blockData = static_cast<TextBlockData*>(currentBlockUserData());
    if(!blockData) {
        blockData = new TextBlockData;
        blockData->brackets.reserve(8);
        setCurrentBlockUserData(blockData);
    }
    else {
        blockData->brackets.clear();
    }

    while (currentIndex < text.size()) {
        switch (currentState) {
        case inCode:
            highlightBlockInCode(text, currentIndex, currentState);
            break;

        case inString:
            highlightBlockInString(text, currentIndex, currentState);
            break;

        case inSymbol:
            highlightBlockInSymbol(text, currentIndex, currentState);
            break;

        default:
            if(currentState >= inComment)
                highlightBlockInComment(text, currentIndex, currentState);
        }
    }
    setCurrentBlockState(currentState);
}

}

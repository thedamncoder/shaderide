/**
 * LogOutputWidget Class
 *
 * --------------------------------------------------------------------------
 * This file is part of "Shader IDE" -> https://github.com/aspicat/shaderide.
 * --------------------------------------------------------------------------
 *
 * Copyright (c) 2019 Aspicat - Florian Roth
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SHADERIDE_GUI_LOGOUTPUTWIDGET_HPP
#define SHADERIDE_GUI_LOGOUTPUTWIDGET_HPP

#include <QPlainTextEdit>
#include <QMenu>
#include <QAction>
#include "src/GUI/Code/SyntaxHighlighter.hpp"
#include "src/GL/GLSLCompileError.hpp"

using namespace ShaderIDE::GL;

namespace ShaderIDE::GUI {

    class LogOutputWidget : public QPlainTextEdit {
    Q_OBJECT
        constexpr static uint16_t MAX_LOG_MESSAGES = 100;

    public:
        explicit LogOutputWidget(QWidget *parent = nullptr);
        ~LogOutputWidget() override;

        void LogMessage(const QString &message);
        void LogSuccessMessage(const QString &message);
        void LogErrorMessage(const QString &message);
        QString LogGLSLError(GLSLCompileError &error);

    protected:
        void contextMenuEvent(QContextMenuEvent *event) override;

    private slots:
        void sl_ClearLog();

    private:
        uint16_t logCounter;

        SyntaxHighlighter *syntaxHighlighter;
        QMenu *contextMenu;
        QAction *clearLogAction;

        void InitFont();
        void InitSyntaxHighlighter();
        void InitContextMenu();

        void DestroyContextMenu();
        void DestroySyntaxHighlighter();

        void AppendPlainTextWithAutomaticCleanup(const QString &message);
    };
}

#endif // SHADERIDE_GUI_LOGOUTPUTWIDGET_HPP

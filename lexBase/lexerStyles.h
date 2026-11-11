#pragma once

#include "lexers.h"

CLexerStyle* GetLexerStyles( CSciWrapper& sci );
void SetupLexerStyles( CSciWrapper& sci );
void SetupLexerStyles( CSciWrapper& sci, CLexerStyle* pStyles ); // global + pStyles

extern CLexerStyle styleFindResults[]; // the find/replace results pane

void SetupIndicators( CSciWrapper& sci );

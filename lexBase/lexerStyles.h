#pragma once

#include "lexers.h"

CLexerStyle* GetLexerStyles( CSciWrapper& sci );
void SetupLexerStyles( CSciWrapper& sci );
void SetupDefaultStyle( CSciWrapper& sci ); // only the global default style, for the find results

void SetupIndicators( CSciWrapper& sci );

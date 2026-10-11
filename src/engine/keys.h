#pragma once

struct KeyInfo_t
{
	char* m_pKeyBinding;
	unsigned char m_nKeyUpTarget : 3;
	unsigned char m_bKeyDown : 1;
};

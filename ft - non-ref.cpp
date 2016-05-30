/* 2006-10-23(by 555)
 * http://hp.vector.co.jp/authors/VA028002/winfreetype.c (higambana(����F�I))
 * ���ێʂ�
 */
/* 2006-10-27(by 555)
 * http://hp.vector.co.jp/authors/VA028002/freetype.html (higambana(����F�I))
 * ���Q�l�ɂ��Ă�蒼��
 */
/* 2006-10-29(by 555)
 * 693��(�ƌĂԂ��Ƃɂ���)�̐��͓I�Ȋ����ɂ���ďo���オ�����E�n�E�n�\�[�X��
 * ��L�T�C�g�̕ύX�_�����ɂ݂݂������C���B(�x�[�Xgdi0164)
 */
/* (by 555)
 * ����ɐ��������E�n�E�n�ɂ��Ă������gdi0168������
 * �C�^���b�N�ƃ{�[���h��ǉ��B
 */
/* (by sy567)
 * �����̃A���S���Y����ύX�B
 * �K���}�␳���������Ă݂�B
 */
#include "override.h"
#include "ft.h"
#include <windows.h>
//#include <windowsx.h>
#include <tchar.h>

#include <math.h>

#include <ft2build.h>
#include <tttables.h>
#include <freetype.h>	/* FT_FREETYPE_H */
#include <ftcache.h>	/* FT_CACHE_H */
//#include <tttags.h>	// FT_TRUETYPE_TAGS_H
//#include <tttables.h>	// FT_TRUETYPE_TABLES_H
#include <ftoutln.h>	// FT_OUTLINE_H
#include <fttrigon.h>	//FT_TRIGONOMETRY_H

#ifdef FT_LCD_FILTER_H
#include <ftlcdfil.h>	// FT_LCD_FILTER_H
#endif

#include "fteng.h"

#include "ft2vert.h"

FT_BitmapGlyphRec empty_glyph = {};//�Ż�������

#define FT_BOLD_LOW 15
#define IsFontBold(lf)		((lf).lfWeight >= FW_BOLD)
#define FT_FixedToInt(x)	(FT_RoundFix(x) >> 16)
#define FT_PosToInt(x)		(((x) + (1 << 5)) >> 6)
#define RESOLUTION_X 72
#define RESOLUTION_Y 72
FT_Error New_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos str_h, FT_Pos str_v, FT_Int font_size);
FT_Error Old_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos strength );
FT_Error Vert_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos strength );
ControlIder CID;

#if _MSC_VER <= 1200
#pragma warning(disable: 4786)
#endif


//����
#define RGBA(r,g,b,a)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)|(((DWORD)(BYTE)(a))<<24)))
//!!Snowie

COLORREF GetPaletteColor(HDC hdc, UINT paletteindex)
{
	//if ((paletteindex>>28)%2) return 0;
	HPALETTE hpal=(HPALETTE)GetCurrentObject(hdc, OBJ_PAL);
	PALETTEENTRY lppe={};
	memset(&lppe, 0, sizeof(lppe));
	GetPaletteEntries(hpal, paletteindex&0xffff, 1, &lppe);
	return RGB(lppe.peRed, lppe.peGreen, lppe.peBlue);
}


void Log(char* Msg)
{	
#ifndef _DEBUG
	return;
#endif
	FILE* f=fopen(".\\gdipp.log","a");
	fputs(Msg, f);
	fclose(f);
}

void Log(wchar_t* Msg)
{	
#ifndef _DEBUG
	return;
#endif
	FILE* f=_wfopen(L".\\gdipp.log",L"a,ccs=UNICODE");
	fputws(Msg, f);
	fclose(f);
}

class CAlphaBlend
{
private:
	int alphatbl[256];
	int tbl1[257];
	BYTE tbl2[256 * 16 + 1];
	// �ʏ�̃A���t�@�l�␳
	int tunetbl[256];
	int tunetblR[256];
	int tunetblG[256];
	int tunetblB[256];
	// �e�����p�̃A���t�@�l�␳
	int tunetblS[256];
	int tunetblRS[256];
	int tunetblGS[256];
	int tunetblBS[256];

	int tunetblLS[256];
	int tunetblLRS[256];
	int tunetblLGS[256];
	int tunetblLBS[256];
	//Snowie!!
	double RGB2CRT[256];	//table used for RGB<->Lab
public:
	static const int BASE;
public:
	CAlphaBlend() { }
	~CAlphaBlend() {}
	void init();
	void initRGB();
	double* GetRGBTable() { return RGB2CRT; }
	BYTE doAB(BYTE fg, BYTE bg, int alpha);
	void gettunetbl(int paramalpha, BOOL lcd, BOOL dark, const int * &tblR, const int * &tblG, const int * &tblB) const;
	inline int conv1(BYTE n) {
		return tbl1[n];
	}
	inline BYTE conv2(int n) {
		return tbl2[n / (BASE * BASE / (sizeof tbl2 - 1))];
	}
private:
	inline int convalpha(int alpha) {
		return alphatbl[alpha];
	}
	inline BYTE rconv1(int n);
};
const int CAlphaBlend::BASE = 0x4000;

static CAlphaBlend s_AlphaBlendTable;

void CAlphaBlend::gettunetbl(int paramalpha, BOOL lcd, BOOL dark, const int * &tblR, const int * &tblG, const int * &tblB) const
{
	if (paramalpha == 1) {	//��ȡ���ֻ�ϱ�
		if (lcd) {
			tblR = tunetblR;
			tblG = tunetblG;
			tblB = tunetblB;
		} else {
			tblR = tblG = tblB = tunetbl;
		}
	} else {	//��ȡ��Ӱ��ϱ�
		if (dark)
		{
			if (lcd) {
				tblR = tunetblRS;
				tblG = tunetblGS;
				tblB = tunetblBS;
			} else {
				tblR = tblG = tblB = tunetblS;
			}
		}
		else
		{
			if (lcd) {
				tblR = tunetblLRS;
				tblG = tunetblLGS;
				tblB = tunetblLBS;
			} else {
				tblR = tblG = tblB = tunetblLS;
			}
		}
	}
}

void CAlphaBlend::initRGB()
{
	for (int i=0; i<256; i++)
		RGB2CRT[i] = pow(i/255.0, 2.2);
}

void CAlphaBlend::init()
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	const float gamma = pSettings->GammaValue();
	const float weight = pSettings->RenderWeight();
	const float contrast = pSettings->Contrast();
	const int mode = pSettings->GammaMode();
	
	int i;
	float temp, alpha;
	
	for (i = 0; i < 256; ++i) {
		temp = pow((1.0f / 255.0f) * i, 1.0f / weight);

		if  (temp < 0.5f) {
			alpha = pow(temp * 2, contrast) / 2.0f;
		} else {
			alpha = 1.0f - pow((1.0f - temp) * 2, contrast) / 2.0f;
		}
		alphatbl[i] = (int)(alpha * BASE);

		if (mode < 0) {
			temp = (1.0f / 255.0f) * i;
		} else {
			if (mode == 1) {
				if (i <= 10) {
					temp = (float)i / (12.92f * 255.0f);
				} else {
					temp = pow(((1.0f / 255.0f) * i + 0.055f) / 1.055f, 2.4f);
				}
			} else if (mode == 2) {
				if (i <= 10) {
					temp = ((float)i / (12.92f * 255.0f) + (float)i / 255.0f) / 2;
				} else {
					temp = (pow(((1.0f / 255.0f) * i + 0.055f) / 1.055f, 2.4f) + (float)i / 255.0f) / 2;
				}
			} else {
				temp = pow((1.0f / 255.0f) * i, gamma);
			}
		}
		tbl1[i] = (int)(temp * BASE);
	}

	tbl1[i] = BASE;

	for (i = 0; i <= sizeof tbl2 - 1; ++i) {
		tbl2[i] = rconv1(i * (BASE / (sizeof tbl2 - 1)));
	}

	const int* table = pSettings->GetTuneTable();
	const int* tableR = pSettings->GetTuneTableR();
	const int* tableG = pSettings->GetTuneTableG();
	const int* tableB = pSettings->GetTuneTableB();
	const int* shadow = pSettings->GetShadowParams();
	const int paramalpha = Max(shadow[2], 1);
	const int lightparamalpha = Max(shadow[3], 1);

	for (i = 0; i < 256; ++i) {
		tunetbl[i] = Bound(0, alphatbl[Bound(table[i], 0, 255)], CAlphaBlend::BASE);
		tunetblR[i] = Bound(0, alphatbl[Bound(tableR[i], 0, 255)], CAlphaBlend::BASE);
		tunetblG[i] = Bound(0, alphatbl[Bound(tableG[i], 0, 255)], CAlphaBlend::BASE);
		tunetblB[i] = Bound(0, alphatbl[Bound(tableB[i], 0, 255)], CAlphaBlend::BASE);
		tunetblS[i] = Bound(0, alphatbl[Bound(table[i] * paramalpha /100, 0, 255)], CAlphaBlend::BASE);
		tunetblRS[i] = Bound(0, alphatbl[Bound(tableR[i] * paramalpha/100, 0, 255)], CAlphaBlend::BASE);
		tunetblGS[i] = Bound(0, alphatbl[Bound(tableG[i] * paramalpha/100, 0, 255)], CAlphaBlend::BASE);
		tunetblBS[i] =Bound(0,  alphatbl[Bound(tableB[i] * paramalpha/100, 0, 255)], CAlphaBlend::BASE);	//ǳɫ��ϱ�

		tunetblLS[i] = Bound(0, alphatbl[Bound(table[i] * lightparamalpha/100, 0, 255)], CAlphaBlend::BASE);
		tunetblLRS[i] = Bound(0, alphatbl[Bound(tableR[i] * lightparamalpha/100, 0, 255)], CAlphaBlend::BASE);
		tunetblLGS[i] = Bound(0, alphatbl[Bound(tableG[i] * lightparamalpha/100, 0, 255)], CAlphaBlend::BASE);
		tunetblLBS[i] =Bound(0,  alphatbl[Bound(tableB[i] * lightparamalpha/100, 0, 255)], CAlphaBlend::BASE);	//��ɫ��ϱ�
	}
}

BYTE CAlphaBlend::rconv1(int n)
{
	int pos = 0x80;
	int i = pos >> 1;
	while (i > 0) {
		if (n >= tbl1[pos]) {
			pos += i;
		} else {
			pos -= i;
		}
		i >>= 1;
	}
	if (n >= tbl1[pos]) {
		++pos;
	}
	return (BYTE)(pos - 1);
}

class CAlphaBlendColorOne
{
private:
	BYTE fg;
	int temp_fg;
	const int *tunetbl;
	BYTE bg0;
	int alpha0;
	BYTE c0;
public:
	CAlphaBlendColorOne()
		: fg(0), temp_fg(0), tunetbl(NULL), bg0(0), alpha0(0), c0(0) {}
	void init(BYTE f, const int *tbl);
	~CAlphaBlendColorOne() {};
	BYTE doAB(BYTE bg, int alpha);
};

FORCEINLINE void CAlphaBlendColorOne::init(BYTE f, const int *tbl)
{
	fg = f;
	temp_fg = s_AlphaBlendTable.conv1(fg);
	tunetbl = tbl;
}

FORCEINLINE BYTE CAlphaBlendColorOne::doAB(BYTE bg, int alpha)
{
	int temp_alpha = tunetbl[alpha];

	return temp_alpha ? s_AlphaBlendTable.conv2(s_AlphaBlendTable.conv1(bg) * (s_AlphaBlendTable.BASE - tunetbl[alpha]) +
				temp_fg * tunetbl[alpha]) : bg;
	
}

class CAlphaBlendColor
{
private:
	CAlphaBlendColorOne r;
	CAlphaBlendColorOne g;
	CAlphaBlendColorOne b;
public:
	CAlphaBlendColor( COLORREF newColor, int paramalpha, BOOL lcd, BOOL dark, bool gbr = false);
	~CAlphaBlendColor() { }
	BYTE doABsub(BYTE fg, int temp_fg, BYTE bg, int temp_alpha) const;
	COLORREF doAB(COLORREF baseColor, int alphaR, int alphaG, int alphaB, BOOL bClearAlpha);
	COLORREF doAB(COLORREF baseColor, int alpha, BOOL bClearAlpha) {
		return doAB(baseColor, alpha, alpha, alpha, bClearAlpha);
	}
private:
	CAlphaBlendColor() { }
};

FORCEINLINE CAlphaBlendColor::CAlphaBlendColor( COLORREF newColor, int paramalpha, BOOL lcd, BOOL dark, bool gbr)
{
	const int *tblR;
	const int *tblG;
	const int *tblB;
	s_AlphaBlendTable.gettunetbl(paramalpha, lcd, dark, tblR, tblG, tblB);
	if (!gbr) {
		r.init(GetRValue(newColor), tblR);
		b.init(GetBValue(newColor), tblB);
	} else {
		r.init(GetBValue(newColor), tblB);
		b.init(GetRValue(newColor), tblR);
	}
	g.init(GetGValue(newColor), tblG);
}

FORCEINLINE COLORREF CAlphaBlendColor::doAB(COLORREF baseColor, int alphaR, int alphaG, int alphaB, BOOL bClearAlpha)
{
	if (alphaB | alphaG | alphaR)
	{
		if (bClearAlpha)
			return RGB(r.doAB(GetRValue(baseColor), alphaR),
					g.doAB(GetGValue(baseColor), alphaG),
					b.doAB(GetBValue(baseColor), alphaB));
		else
			return RGBA(r.doAB(GetRValue(baseColor), alphaR),
					g.doAB(GetGValue(baseColor), alphaG),
					b.doAB(GetBValue(baseColor), alphaB),
					baseColor>>24);
	}
	else
		return baseColor;
}

FORCEINLINE BYTE CAlphaBlend::doAB(BYTE fg, BYTE bg, int alpha)
{
	if (fg == bg || alpha <= 0) return bg;
	if (alpha >= 255) return fg;
	int temp_alpha = convalpha(alpha);
	int temp_bg = conv1(bg);
	int temp_fg = conv1(fg);
	int temp = temp_bg * (BASE - temp_alpha) +
			temp_fg * temp_alpha;
	return conv2(temp);
}

FORCEINLINE BYTE DoAlphaBlend(BYTE fg, BYTE bg, int alpha)
{
	return s_AlphaBlendTable.doAB(fg, bg, alpha);
}

// LCD(�t��)�p�̃A���t�@�u�����h(�T�u�s�N�Z�������_�����O)
static FORCEINLINE
COLORREF AlphaBlendColorLCD(
		COLORREF baseColor,
		COLORREF newColor,
		int alphaR, int alphaG, int alphaB,
		const int* tableR, const int* tableG, const int* tableB,
		const FreeTypeDrawInfo& ftdi)
{
	const BYTE rs = GetRValue(baseColor);
	const BYTE gs = GetGValue(baseColor);
	const BYTE bs = GetBValue(baseColor);
	BYTE rd = GetRValue(newColor);
	BYTE gd = GetGValue(newColor);
	BYTE bd = GetBValue(newColor);
	// �A���t�@�l��␳
	alphaR = tableR[alphaR] / ftdi.params->alpha;
	alphaG = tableG[alphaG] / ftdi.params->alpha;
	alphaB = tableB[alphaB] / ftdi.params->alpha;
//	rd = (((rd - rs) * alphaR) / 255) + rs;
//	gd = (((gd - gs) * alphaG) / 255) + gs;
//	bd = (((bd - bs) * alphaB) / 255) + bs;
	rd = DoAlphaBlend(rd, rs, alphaR);
	gd = DoAlphaBlend(gd, gs, alphaG);
	bd = DoAlphaBlend(bd, bs, alphaB);
	return RGB(rd, gd, bd);
}

// �A���t�@�u�����h(256�K��)
static FORCEINLINE
COLORREF AlphaBlendColor(
		COLORREF baseColor,
		COLORREF newColor,
		int alpha, const int* table,
		const FreeTypeDrawInfo& ftdi)
{
	const BYTE rs = GetRValue(baseColor);
	const BYTE gs = GetGValue(baseColor);
	const BYTE bs = GetBValue(baseColor);
	BYTE rd = GetRValue(newColor);
	BYTE gd = GetGValue(newColor);
	BYTE bd = GetBValue(newColor);
	// �A���t�@�l��␳
	alpha = table[alpha] / ftdi.params->alpha;
//	rd = (rs * (255 - alpha) + rd * alpha) / 255;
//	gd = (gs * (255 - alpha) + gd * alpha) / 255;
//	bd = (bs * (255 - alpha) + bd * alpha) / 255;

//	rd = (((rd - rs) * alpha) / 255) + rs;
//	gd = (((gd - gs) * alpha) / 255) + gs;
//	bd = (((bd - bs) * alpha) / 255) + bs;
	rd = DoAlphaBlend(rd, rs, alpha);
	gd = DoAlphaBlend(gd, gs, alpha);
	bd = DoAlphaBlend(bd, bs, alpha);
	return RGB(rd, gd, bd);
}

typedef struct  
{
	FreeTypeDrawInfo* FTInfo;			//orignal draw information
	WCHAR wch;							//text to draw
	FT_BitmapGlyph FTGlyph;			//glyph
	int	AAMode;							//antialiased mode for every char
	CAlphaBlendColor* solid;
	CAlphaBlendColor* shadow;	//alpha blender
} FreeTypeGlyphInfo, *PFreeTypeGlyphInfo;


// 2�K��
static void FreeTypeDrawBitmapPixelModeMono(FreeTypeGlyphInfo& FTGInfo,
											CAlphaBlendColor& ab, int x, int y)
{
	CBitmapCache& cache = *FTGInfo.FTInfo->pCache;
	const FT_Bitmap *bitmap = &FTGInfo.FTGlyph->bitmap;
	BYTE alphatuner = FTGInfo.FTInfo->params->alphatuner;
	int i, j;
	int dx, dy;	// display
	FT_Bytes p;

	if(bitmap->pixel_mode != FT_PIXEL_MODE_MONO){
		return;
	}

	const COLORREF color = RGB2DIB(FTGInfo.FTInfo->Color());

	const SIZE cachebufsize = cache.Size();
	DWORD * const cachebufp = (DWORD *)cache.GetPixels();
	DWORD * cachebufrowp;

	int left, top, width, height;
	if (x < 0) {
		left = -x;
		x = 0;
	} else {
		left = 0;
	}
	width = Min((int)bitmap->width, (int)(cachebufsize.cx - x));
	top = 0;
	height = bitmap->rows;

	for(j = top, dy = y; j < height; ++j, ++dy){
		if ((unsigned int)dy >= (unsigned int)cachebufsize.cy) continue;
		p = bitmap->pitch < 0 ?
			&bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :	// up-flow
			&bitmap->buffer[bitmap->pitch * j];	// down-flow
		cachebufrowp = &cachebufp[dy * cachebufsize.cx];
		for(i = left, dx = x; i < width; ++i, ++dx){
			if((p[i / 8] & (1 << (7 - (i % 8)))) != 0){
				cachebufrowp[dx] = color;
			}
		}
	}
}

// LCD(�t��)�p�`��(�T�u�s�N�Z�������_�����O)
// RGB��(�̂͂�)
static void FreeTypeDrawBitmapPixelModeLCD(FreeTypeGlyphInfo& FTGInfo,
		CAlphaBlendColor& ab, int x, int y)
{
	CBitmapCache& cache = *FTGInfo.FTInfo->pCache;
	const FT_Bitmap *bitmap = &FTGInfo.FTGlyph->bitmap;
	BYTE alphatuner = FTGInfo.FTInfo->params->alphatuner;
	int AAMode = FTGInfo.AAMode;
	int i, j;
	int dx, dy;	// display
	FT_Bytes p;

	if(bitmap->pixel_mode != FT_PIXEL_MODE_LCD){
		return;
	}

	const COLORREF color = FTGInfo.FTInfo->Color();

	const SIZE cachebufsize = cache.Size();
	DWORD * const cachebufp = (DWORD *)cache.GetPixels();
	DWORD * cachebufrowp;

	// LCD��3�T�u�s�N�Z��������
	int left, top, width, height;
	if (x < 0) {
		left = -x * 3;
		x = 0;
	} else {
		left = 0;
	}
	width = Min((int)bitmap->width, (int)(cachebufsize.cx - x) * 3);
	top = 0;
	height = bitmap->rows;
	//CAlphaBlendColor ab(color, ftdi.params->alpha, true, true);

	COLORREF backColor, newColor;
	unsigned int alphaR, alphaG, alphaB;
	BOOL bAlphaDraw = FTGInfo.FTInfo->params->alpha!=1;

	if (bAlphaDraw)
		for(j = 0, dy = y; j < height; ++j, ++dy){
		if ((unsigned int)dy >= (unsigned int)cachebufsize.cy) continue;

		p = bitmap->pitch < 0 ?
			&bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :	// up-flow
			&bitmap->buffer[bitmap->pitch * j];	// down-flow

		cachebufrowp = &cachebufp[dy * cachebufsize.cx];
		for(i = left, dx = x; i < width; i += 3, ++dx){
			backColor = cachebufrowp[dx];
			COLORREF last=0xFFFFFFFF;
			if(AAMode == 2 || AAMode == 4){
				// �����RGB�̏��ɃT�u�s�N�Z��������f�B�X�v���C�p
				// �����RGB�̏��ɃT�u�s�N�Z��������f�B�X�v���C�p
				alphaR = p[i + 0] / alphatuner; 
				alphaG = p[i + 1] / alphatuner;
				alphaB = p[i + 2] / alphatuner;
			}else{
				// BGR
				alphaR = p[i + 2] / alphatuner;
				alphaG = p[i + 1] / alphatuner;
				alphaB = p[i + 0] / alphatuner;
			}
/*
			if (bAlphaDraw)
			{
				if (alphaB && alphaG && alphaR)
					backColor &= 0x00ffffff;
			}
			else*/

			//if ((alphaB || alphaG || alphaR))
			//	backColor &= 0x00ffffff;
			 newColor = ab.doAB(backColor, alphaB, alphaG, alphaR, !bAlphaDraw);
			 cachebufrowp[dx] = newColor;
		}
	}
	else
	for(j = 0, dy = y; j < height; ++j, ++dy){
		if ((unsigned int)dy >= (unsigned int)cachebufsize.cy) continue;

		p = bitmap->pitch < 0 ?
			&bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :	// up-flow
			&bitmap->buffer[bitmap->pitch * j];	// down-flow

		cachebufrowp = &cachebufp[dy * cachebufsize.cx];
		for(i = left, dx = x; i < width; i += 3, ++dx){
			backColor = cachebufrowp[dx];
			COLORREF last=0xFFFFFFFF;
			if(AAMode == 2 || AAMode == 4){
				// �����RGB�̏��ɃT�u�s�N�Z��������f�B�X�v���C�p
				// �����RGB�̏��ɃT�u�s�N�Z��������f�B�X�v���C�p
				alphaR = p[i + 0]; 
				alphaG = p[i + 1];
				alphaB = p[i + 2];
			}else{
				// BGR
				alphaR = p[i + 2];
				alphaG = p[i + 1];
				alphaB = p[i + 0];
			}
/*
			if (bAlphaDraw)
			{
				if (alphaB && alphaG && alphaR)
					backColor &= 0x00ffffff;
			}
			else*/

			//if ((alphaB || alphaG || alphaR))
			//	backColor &= 0x00ffffff;
			 newColor = ab.doAB(backColor, alphaB, alphaG, alphaR, !bAlphaDraw);
			 cachebufrowp[dx] = newColor;
		}
	}
}

static void FreeTypeDrawBitmapGray(FreeTypeGlyphInfo& FTGInfo, CAlphaBlendColor& ab, int x, int y)
{
	int i, j;
	int dx, dy;	// display
	COLORREF c;
	FT_Bytes p;

	CBitmapCache& cache = *FTGInfo.FTInfo->pCache;
	const FT_Bitmap *bitmap = &FTGInfo.FTGlyph->bitmap;
	BYTE alphatuner = FTGInfo.FTInfo->params->alphatuner;

	BOOL bAlphaDraw = FTGInfo.FTInfo->params->alpha!=1;
	const COLORREF color = FTGInfo.FTInfo->Color();
	const SIZE cachebufsize = cache.Size();
	DWORD * const cachebufp = (DWORD *)cache.GetPixels();
	DWORD * cachebufrowp;

	int left, top, width, height;
	if (x < 0) {
		left = -x;
		x = 0;
	} else {
		left = 0;
	}
	width = Min((int)bitmap->width, (int)(cachebufsize.cx - x));
	top = 0;
	height = bitmap->rows;

	//	CAlphaBlendColor ab(color, ftdi.params->alpha, false, true);

	COLORREF backColor;
	int alpha;

	for(j = top, dy = y; j < height; ++j, ++dy){
		if ((unsigned int)dy >= (unsigned int)cachebufsize.cy) continue;
		p = bitmap->pitch < 0 ?
			&bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :	// up-flow
		&bitmap->buffer[bitmap->pitch * j];	// down-flow
		cachebufrowp = &cachebufp[dy * cachebufsize.cx];
		for(i = left, dx = x; i < width; ++i, ++dx){
			alpha = p[i];
			backColor = cachebufrowp[dx];
			c = ab.doAB(backColor, alpha, !bAlphaDraw);
			cachebufrowp[dx] = c;
		}
	}
}

// �O���t�r�b�g�}�b�v�̃����_�����O
static void FreeTypeDrawBitmap(
		FreeTypeGlyphInfo& FTGInfo,
		CAlphaBlendColor& ab,
		int x, int y)
{
	if(FTGInfo.FTGlyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY){
		// ���̊֐����̂�FT_PIXEL_MODE_GRAY�ɂ̂ݑΉ������ɈϏ�����
		switch(FTGInfo.FTGlyph->bitmap.pixel_mode){
		case FT_PIXEL_MODE_MONO:
			FreeTypeDrawBitmapPixelModeMono(FTGInfo, ab, x, y);
			break;
		case FT_PIXEL_MODE_LCD:
			FreeTypeDrawBitmapPixelModeLCD(FTGInfo, ab, x, y);
			break;
		default:
			return;		// ���Ή�
		}
		return;
	}
	FreeTypeDrawBitmapGray(FTGInfo, ab, x, y);
}

// �c�����p�̃����_�����O(�R�s�y�蔲��)
// 2�K��
static void FreeTypeDrawBitmapPixelModeMonoV(FreeTypeGlyphInfo& FTGInfo,
											 CAlphaBlendColor& ab, int x, int y)
{
	CBitmapCache& cache = *FTGInfo.FTInfo->pCache;
	FT_Bitmap *bitmap = &FTGInfo.FTGlyph->bitmap;
	int i, j;
	int dx, dy;	// display
	FT_Bytes p;

	if(bitmap->pixel_mode != FT_PIXEL_MODE_MONO){
		return;
	}

	const COLORREF color = FTGInfo.FTInfo->Color();

	const int width = bitmap->width;
	const int height = bitmap->rows;

	for(j = 0, dy = x; j < height; ++j, ++dy){
		p = bitmap->pitch < 0 ?
			&bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :	// up-flow
			&bitmap->buffer[bitmap->pitch * j];	// down-flow
		for(i = 0, dx = y+width; i < width; ++i, --dx){
			if((p[i / 8] & (1 << (7 - (i % 8)))) != 0){
				if (cache.GetPixel(dx, dy) != CLR_INVALID) { // dx dy �G���[�`�F�b�N
					cache.SetCurrentPixel(color);
				}
			}
		}
	}
}

// LCD(�t��)�p�`��(�T�u�s�N�Z�������_�����O)
// RGB��(�̂͂�)
static void FreeTypeDrawBitmapPixelModeLCDV(FreeTypeGlyphInfo& FTGInfo,
											CAlphaBlendColor& ab, int x, int y)
{
	CBitmapCache& cache = *FTGInfo.FTInfo->pCache;
	const FT_Bitmap *bitmap = &FTGInfo.FTGlyph->bitmap;
	BYTE alphatuner = FTGInfo.FTInfo->params->alphatuner;
	int AAMode = FTGInfo.AAMode;
	int i, j;
	int dx, dy;	// display
	COLORREF c;
	FT_Bytes p;

	if(bitmap->pixel_mode != FT_PIXEL_MODE_LCD_V){
		return;
	}

	const COLORREF color = FTGInfo.FTInfo->Color();

	// LCD��3�T�u�s�N�Z��������
	const int width = bitmap->width;
	const int height = bitmap->rows;
	const int pitch = bitmap->pitch;
	const int pitchabs = pitch < 0 ? -pitch : pitch;
	BOOL bAlphaDraw = FTGInfo.FTInfo->params->alpha!=1;
	//CAlphaBlendColor ab(color, ftdi.params->alpha, true);

	if (bAlphaDraw)
		for(j = 0, dy = x; j < height; j += 3, ++dy){
			p = pitch < 0 ?
				&bitmap->buffer[(pitchabs * bitmap->rows) + pitchabs * j] :	// up-flow
				&bitmap->buffer[pitchabs * j];	// down-flow

			int alphaR, alphaG, alphaB;
			for(i = 0, dx = y+width; i < width; ++i, --dx){
				COLORREF backColor = cache.GetPixel(dy, dx);

				if (backColor == color || backColor == CLR_INVALID) continue;
				if(AAMode == 2 || AAMode == 4){
					// �����RGB�̏��ɃT�u�s�N�Z��������f�B�X�v���C�p
					alphaR = p[i + 0] / alphatuner;
					alphaG = p[i + pitch] / alphatuner;
					alphaB = p[i + pitch * 2] / alphatuner;
				}else{
					// BGR
					alphaR = p[i + pitch * 2]/ alphatuner;
					alphaG = p[i + pitch]/ alphatuner;
					alphaB = p[i + 0]/ alphatuner;
				}

				c = ab.doAB(backColor, alphaR, alphaG, alphaB, !bAlphaDraw);
				cache.SetCurrentPixel(c);
			}

			if (i >= width)
				continue;
		}
	else
		for(j = 0, dy = x; j < height; j += 3, ++dy){
			p = pitch < 0 ?
				&bitmap->buffer[(pitchabs * bitmap->rows) + pitchabs * j] :	// up-flow
			&bitmap->buffer[pitchabs * j];	// down-flow

			int alphaR, alphaG, alphaB;
			for(i = 0, dx = y+width; i < width; ++i, --dx){
				COLORREF backColor = cache.GetPixel(dy, dx);

				if (backColor == color || backColor == CLR_INVALID) continue;
				if(AAMode == 2 || AAMode == 4){
					// �����RGB�̏��ɃT�u�s�N�Z��������f�B�X�v���C�p
					alphaR = p[i + 0];
					alphaG = p[i + pitch];
					alphaB = p[i + pitch * 2];
				}else{
					// BGR
					alphaR = p[i + pitch * 2];
					alphaG = p[i + pitch];
					alphaB = p[i + 0];
				}

				c = ab.doAB(backColor, alphaR, alphaG, alphaB, !bAlphaDraw);
				cache.SetCurrentPixel(c);
			}

			if (i >= width)
				continue;
		}
}

void FreeTypeDrawBitmapGrayV(FreeTypeGlyphInfo& FTGInfo, CAlphaBlendColor& ab, int x, int y)
{
	CBitmapCache& cache = *FTGInfo.FTInfo->pCache;
	const FT_Bitmap *bitmap = &FTGInfo.FTGlyph->bitmap;
	BYTE alphatuner = FTGInfo.FTInfo->params->alphatuner;
	int i, j;
	int dx, dy;	// display
	int width, height;
	COLORREF c;
	FT_Bytes p;


	const COLORREF color = FTGInfo.FTInfo->Color();
	//const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	//const int* table = pSettings->GetTuneTable();
	width = bitmap->width;
	height = bitmap->rows;

	//	CAlphaBlendColor ab(color, ftdi.params->alpha, false);

	for(j = 0, dy = x; j < height; ++j, ++dy){
		p = bitmap->pitch < 0 ?
			&bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :	// up-flow
		&bitmap->buffer[bitmap->pitch * j];	// down-flow
		for(i = 0, dx = y+width; i < width; ++i, --dx){
			const COLORREF backColor = cache.GetPixel(dy, dx);
			if (backColor == color || backColor == CLR_INVALID) continue;
			c = ab.doAB(backColor, p[i], true);
			cache.SetPixelV(dy, dx, c);
		}
	}
}

static void FreeTypeDrawBitmapV(FreeTypeGlyphInfo& FTGInfo, CAlphaBlendColor& ab, const int x, const int y)
{
	if(FTGInfo.FTGlyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY){
		// ���̊֐����̂�FT_PIXEL_MODE_GRAY�ɂ̂ݑΉ������ɈϏ�����
		switch(FTGInfo.FTGlyph->bitmap.pixel_mode){
		case FT_PIXEL_MODE_MONO:
			FreeTypeDrawBitmapPixelModeMonoV(FTGInfo, ab, x, y);
			break;
		case FT_PIXEL_MODE_LCD_V:
			FreeTypeDrawBitmapPixelModeLCDV(FTGInfo, ab, x, y);
			break;
		default:
			return;		// ���Ή�
		}
		return;
	}
}

class CGGOGlyphLoader
{
private:
	FT_Library m_lib;
	const FT_Glyph_Class *m_clazz;
	BYTE bgtbl[0x41];
	static int CALLBACK EnumFontFamProc(const LOGFONT* lplf, const TEXTMETRIC* lptm, DWORD FontType, LPARAM lParam);
public:
	CGGOGlyphLoader() : m_lib(NULL), m_clazz(NULL) {}
	~CGGOGlyphLoader() {}
	bool init(FT_Library freetype_library);
	FT_Library getlib() { return m_lib; }
	const FT_Glyph_Class * getclazz() { return m_clazz; }
	BYTE convbgpixel(BYTE val) { return bgtbl[val]; }
};
static CGGOGlyphLoader s_GGOGlyphLoader;

int CALLBACK CGGOGlyphLoader::EnumFontFamProc(const LOGFONT* lplf, const TEXTMETRIC* lptm, DWORD FontType, LPARAM lParam)
{
	CGGOGlyphLoader* pThis = reinterpret_cast<CGGOGlyphLoader*>(lParam);
	if (FontType != TRUETYPE_FONTTYPE || lplf->lfCharSet == SYMBOL_CHARSET) {
		return TRUE;
	}

	TRACE(_T("Face: %s\n"), lplf->lfFaceName);
	FreeTypeSysFontData* pFont = FreeTypeSysFontData::CreateInstance(lplf->lfFaceName, 0, false);
	if (!pFont) {
		return TRUE;
	}

	const FT_Glyph_Class *clazz = NULL;
	FT_Face face = pFont->GetFace();
	FT_Error err = FT_Set_Pixel_Sizes(face, 0, 12);//optimized
	if (!err) {
		err = FT_Load_Char(face, lptm->tmDefaultChar, FT_LOAD_NO_BITMAP);
		if (!err) {
			FT_Glyph glyph;
			err = FT_Get_Glyph(face->glyph, &glyph);
			if (!err) {
				if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
					clazz = glyph->clazz;
				}
				FT_Done_Glyph(glyph);
			}
		}
	}

	FT_Done_Face(face);

	if (clazz) {
		pThis->m_clazz = clazz;
		//�񋓒��~
		return FALSE;
	}
	return TRUE;
}

bool
CGGOGlyphLoader::init(FT_Library freetype_library)
{
	if (m_lib) {
		return true;
	}
	
	if (!freetype_library) {
		return false;
	}

	for (BYTE val = 0; val <= 0x40; ++val) {
		BYTE t = (BYTE)(((DWORD)val * 256) / 65);
		bgtbl[val] = t + (t >> 6);
	}

	m_lib = freetype_library;
	m_clazz = NULL;

	//�O�̕��@���ƁAarial.ttf�������Ƃ܂������Ȃ̂�
	//�K���Ɏg����A�E�g���C���t�H���g��T��
	HDC hdc = CreateCompatibleDC(NULL);
	EnumFontFamilies(hdc, NULL, EnumFontFamProc, reinterpret_cast<LPARAM>(this));
	DeleteDC(hdc);

	if (m_clazz != NULL) {
		return true;
	}
	m_lib = NULL;
	return false;
}

class CGGOOutlineGlyph
{
private:
	FT_OutlineGlyph m_ptr;
	static FT_F26Dot6 toF26Dot6(const FIXED& fx) {
		return *(LONG *)(&fx) >> 10;
	}
	static FT_Fixed toFixed(const short n) {
		return (FT_Fixed)n << 16;
	}
	static char getTag(char tag, const FT_Vector& point) {
		if ((point.x & 0x0f) != 0) {
			tag |= FT_CURVE_TAG_TOUCH_X;
		}
		if ((point.y & 0x0f) != 0) {
			tag |= FT_CURVE_TAG_TOUCH_Y;
		}
		return tag;
	}
public:
	CGGOOutlineGlyph() : m_ptr(NULL) { _ASSERTE(s_GGOGlyphLoader.getlib()); }
	~CGGOOutlineGlyph() { done(); };
	bool init(DWORD bufsize, PVOID bufp, const GLYPHMETRICS& gm);
	void done();
	operator FT_Glyph () { return (FT_Glyph)m_ptr; }
};

void
CGGOOutlineGlyph::done()
{
	if (m_ptr) {
		free(m_ptr->outline.points);
		free(m_ptr->outline.tags);
		free(m_ptr->outline.contours);
	}
	free(m_ptr);
	m_ptr = NULL;
}

bool
CGGOOutlineGlyph::init(DWORD bufsize, PVOID bufp, const GLYPHMETRICS& gm)
{
	done();
	m_ptr = (FT_OutlineGlyph)calloc(1, sizeof *m_ptr);
	if (!m_ptr) {
		return false;
	}

	FT_GlyphRec& root = m_ptr->root;
	FT_Outline& outline = m_ptr->outline;

	root.library = s_GGOGlyphLoader.getlib();
	root.clazz = s_GGOGlyphLoader.getclazz();
	root.format = FT_GLYPH_FORMAT_OUTLINE;
	root.advance.x = toFixed(gm.gmCellIncX);
	root.advance.y = toFixed(gm.gmCellIncY);

	outline.n_contours = 0;
	outline.n_points = 0;
	outline.flags = 0; //FT_OUTLINE_HIGH_PRECISION;

	LPTTPOLYGONHEADER ttphp = (LPTTPOLYGONHEADER)bufp;
	LPTTPOLYGONHEADER ttphpend = (LPTTPOLYGONHEADER)((PBYTE)ttphp + bufsize);

	while (ttphp < ttphpend) {
		LPTTPOLYCURVE ttpcp = (LPTTPOLYCURVE)(ttphp + 1);
		LPTTPOLYCURVE ttpcpend = (LPTTPOLYCURVE)((PBYTE)ttphp + ttphp->cb);
		if ((PVOID)ttpcpend > (PVOID)ttphpend) {
			break;
		}
		++outline.n_points;
		++outline.n_contours;
		while (ttpcp < ttpcpend) {
			LPPOINTFX pfxp = &ttpcp->apfx[0];
			outline.n_points += ttpcp->cpfx;
			ttpcp = (LPTTPOLYCURVE)(pfxp + ttpcp->cpfx);
		}
		ttphp = (LPTTPOLYGONHEADER)ttpcp;
	}

	if (ttphp != ttphpend) {
		return false;
	}
	outline.points = (FT_Vector *)calloc(outline.n_points, sizeof *outline.points);
	outline.tags = (char *)calloc(outline.n_points, sizeof *outline.tags);
	outline.contours = (short *)calloc(outline.n_contours, sizeof *outline.contours);
	if (!outline.points || !outline.tags || !outline.contours) {
		done();
		return false;
	}

	short *cp = outline.contours;
	short ppos = 0;

	ttphp = (LPTTPOLYGONHEADER)bufp;
	while (ttphp < ttphpend) {
		LPTTPOLYCURVE ttpcp = (LPTTPOLYCURVE)(ttphp + 1);
		LPTTPOLYCURVE ttpcpend = (LPTTPOLYCURVE)((PBYTE)ttphp + ttphp->cb);

		LPPOINTFX pfxp0 = &ttpcp->apfx[0];
		while (ttpcp < ttpcpend) {
			LPPOINTFX pfxp = &ttpcp->apfx[0];
			pfxp0 = pfxp + (ttpcp->cpfx - 1);
			ttpcp = (LPTTPOLYCURVE)(pfxp + ttpcp->cpfx);
		}
		ttpcp = (LPTTPOLYCURVE)(ttphp + 1);

		if (pfxp0->x.value != ttphp->pfxStart.x.value || pfxp0->x.fract != ttphp->pfxStart.x.fract ||
			pfxp0->y.value != ttphp->pfxStart.y.value || pfxp0->y.fract != ttphp->pfxStart.y.fract) {
			outline.points[ppos].x = toF26Dot6(ttphp->pfxStart.x);
			outline.points[ppos].y = toF26Dot6(ttphp->pfxStart.y);
			outline.tags[ppos] = getTag(FT_CURVE_TAG_ON, outline.points[ppos]);
			++ppos;
		}
		while (ttpcp < ttpcpend) {
			char tag;
			switch (ttpcp->wType) {
			case TT_PRIM_LINE:
				tag = FT_CURVE_TAG_ON;
				break;
			case TT_PRIM_QSPLINE:
				tag = FT_CURVE_TAG_CONIC;
				break;
			case TT_PRIM_CSPLINE:
				tag = FT_CURVE_TAG_CONIC;
				break;
			default:
				tag = 0;
			}

			LPPOINTFX pfxp = &ttpcp->apfx[0];
			for (WORD cnt = 0; cnt < ttpcp->cpfx; ++cnt) {
				outline.points[ppos].x = toF26Dot6(pfxp->x);
				outline.points[ppos].y = toF26Dot6(pfxp->y);
				outline.tags[ppos] = tag;
				++ppos;
				++pfxp;
			}
			outline.tags[ppos - 1] = getTag(FT_CURVE_TAG_ON, outline.points[ppos - 1]);
			ttpcp = (LPTTPOLYCURVE)pfxp;
		}
		*cp++ = ppos - 1;
		ttphp = (LPTTPOLYGONHEADER)ttpcp;
	}
	outline.n_points = ppos;
	return true;
}

template<typename T>
class CTempMem
{
private:
	char m_localbuf[0x0f80];
	DWORD m_size;
	T m_ptr;
public:
	CTempMem() : m_size(sizeof m_localbuf), m_ptr((T)m_localbuf) {
	}
	~CTempMem() {
		done();
	}
	T init(DWORD size) {
		done();
		if (m_size > size) {
			m_size = size;
			m_ptr = (T)malloc(m_size);
		}
		return m_ptr;
	}
	void done() {
		if (m_ptr != (T)m_localbuf) {
			free(m_ptr);
		}
		m_size = sizeof m_localbuf;
		m_ptr = (T)m_localbuf;
	}
	operator T () { return m_ptr; }
	bool operator ! () { return !m_ptr; }
	DWORD getsize() { return m_size; }
};

BOOL FreeTypePrepare(FreeTypeDrawInfo& FTInfo)
{
//CDebugElapsedCounter cntr("FreeTypePrepare");
#ifdef _DEBUG
	FTInfo.Validate();
#endif

	FT_Face& freetype_face			= FTInfo.freetype_face;
	FT_Int& cmap_index				= FTInfo.cmap_index;
	FT_Render_Mode& render_mode		= FTInfo.render_mode;
	FTC_ImageTypeRec& font_type		= FTInfo.font_type;
	FreeTypeFontInfo*& pfi			= FTInfo.pfi;
	const CFontSettings*& pfs		= FTInfo.pfs;
	FreeTypeFontCache*& pftCache	= FTInfo.pftCache;
	FTC_ScalerRec& scaler			= FTInfo.scaler;
	TEXTMETRIC& tm					= FTInfo.params->otm->otmTextMetrics;

	FTC_FaceID face_id = NULL;
	int height = 0;

	const LOGFONTW& lf = FTInfo.LogFont();
	render_mode		= FT_RENDER_MODE_NORMAL;
	if (FTInfo.params->alpha < 1)
		FTInfo.params->alpha = 1;

	if (lf.lfFaceName == NULL)
		return FALSE;	//optimized
	FTInfo.face_id_list_num = 0;
	//Assert(_tcsicmp(lf.lfFaceName, _T("@Arial Unicode MS")) != 0);
	pfi = NULL;
	CGdippSettings* pSettings = CGdippSettings::GetInstance();
	const bool bVertical = pSettings->FontLoader()==SETTING_FONTLOADER_FREETYPE? lf.lfFaceName[0] == _T('@'):false;

	FreeTypeFontInfo* pfitemp = g_pFTEngine->FindFont(FTInfo.params);
	if (pfitemp) {
		if (!pfi) pfi = pfitemp;
		FTInfo.face_id_list_num = pfi->GetFTLink(&FTInfo.face_id_list);
		pfi->GetGGOLink(&FTInfo.ggo_font_list);
		FTInfo.face_id_simsun = pfi->GetSimSunID();
	}
	if (!(freetype_face = FTInfo.GetFace(0)))
	{
		pSettings->AddFontExclude(lf.lfFaceName);
		return FALSE;
	}

	if (!pfi) {
		return FALSE;
	}

	FTInfo.params->lplf->lfWeight = FTInfo.params->otm->otmTextMetrics.tmWeight;	//���µ���׼weight
	pfs = &pfi->GetFontSettings();

	cmap_index = -1;	
	switch (pSettings->FontLoader()) {
	case SETTING_FONTLOADER_FREETYPE:
		{
			face_id = (FTC_FaceID)pfi->GetId();

			scaler.face_id	= face_id;

			height = FTInfo.params->otm->otmTextMetrics.tmHeight - FTInfo.params->otm->otmTextMetrics.tmInternalLeading;	//Snowie!!�����հ׸߶ȣ�bugfix��
// 				if(lf.lfHeight > 0){
// 					scaler.height = height;
// 				}
// 				else{
			scaler.height = height;
//				}
			//Snowie!!
			TT_OS2* os2_table = pfitemp->GetOS2Table();

			if (lf.lfQuality && os2_table->xAvgCharWidth)
			{
				if (!(freetype_face->style_flags & FT_STYLE_FLAG_BOLD) && tm.tmWeight>= FW_BOLD)
					--FTInfo.params->otm->otmTextMetrics.tmAveCharWidth;
				scaler.width = MulDiv(FTInfo.params->otm->otmTextMetrics.tmAveCharWidth, FTInfo.params->otm->otmEMSquare, os2_table->xAvgCharWidth);
			}
			else
				scaler.width = scaler.height;
			if (bVertical)
				swap(scaler.width, scaler.height);//������������壬��������
			//!!Snowie
			scaler.pixel = 1;
			scaler.x_res = 0;
			scaler.y_res = 0;
/*
			FT_Size font_size;
			{
				CCriticalSectionLock __lock(CCriticalSectionLock::CS_MANAGER);
				if(FTC_Manager_LookupSize(cache_man, &scaler, &font_size))
					return FALSE;
			}*/
			height = scaler.height;
			break;
		}
	case SETTING_FONTLOADER_WIN32:
		{
			/*
			OUTLINETEXTMETRIC otm;
						if (GetOutlineTextMetrics(FTInfo.hdc, sizeof otm, &otm) != sizeof otm) {
							return FALSE;
						}*/
			height = -lf.lfHeight;
			scaler.height = height;
			scaler.width = lf.lfWidth;
		}
		break;
	default:
		return FALSE;
	}
// 	if (!(freetype_face = FTInfo.GetFace(0)))
// 	{
// 		pSettings->AddFontExclude(lf.lfFaceName);
// 		return FALSE;
// 	}

	pftCache = pfi->GetCache(scaler, lf);
	if(!pftCache)
		return FALSE;

	/*FT_Size_RequestRec size_request;
	size_request.width = lf.lfWidth;
	size_request.horiResolution = 0;
	size_request.vertResolution = 0;
	if(lf.lfHeight > 0){
		// �Z������
		size_request.type = FT_SIZE_REQUEST_TYPE_CELL;
		size_request.height = lf.lfHeight * 64;
	}
	else{
		// ��������
		size_request.type = FT_SIZE_REQUEST_TYPE_NOMINAL;
		size_request.height = (-lf.lfHeight) * 64;
	}
	if(FT_Request_Size(freetype_face, &size_request))
		goto Exit2;*/

	switch (pSettings->FontLoader()) {
	case SETTING_FONTLOADER_FREETYPE:
		// font_type��ݒ�
		font_type.face_id = face_id;
		font_type.width   = scaler.width;//freetype_face->size->metrics.x_ppem;
		font_type.height  = scaler.height;//freetype_face->size->metrics.y_ppem;
		//Snowie!!
		FTInfo.height = font_type.height;
		FTInfo.width = font_type.width;

		/* �r�b�g�}�b�v�܂ŃL���b�V������ꍇ��FT_LOAD_RENDER | FT_LOAD_TARGET_*
		 * �Ƃ���B�������r����TARGET��ύX�����ꍇ���̓L���b�V�����ז�����B
		 * ������������FT_LOAD_DEFAULT�ɂ���FTC_ImageCache_Lookup���
		 * FT_Glyph_To_Bitmap�����ق����s���������Ǝv���B
		 */
		// Bold�͑����Ƃ������̂�����̂Ŗ{���͂��ꂾ���ł͑���Ȃ��C������B
		/*if(IsFontBold(lf) && !(freetype_face->style_flags & FT_STYLE_FLAG_BOLD) ||
			lf.lfItalic && !(freetype_face->style_flags & FT_STYLE_FLAG_ITALIC)){
			// �{�[���h�A�C�^���b�N�͌�Ń����_�����O����
			// �������x�͗򉻂��邾�낤���ǎd���Ȃ��B
			font_type.flags = FT_LOAD_NO_BITMAP;
		}
		else{
			font_type.flags = FT_LOAD_RENDER | FT_LOAD_NO_BITMAP;
		}*/
		break;
	case SETTING_FONTLOADER_WIN32:
		font_type.face_id = face_id;
		font_type.width   = -1;
		font_type.height  = -1;
		break;

	DEFAULT_UNREACHABLE;
	}
	font_type.flags = FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;

	// �q���e�B���O
	switch (pfs->GetHintingMode()) {
	case 0:
		// ignore.
		break;
	case 1:
		font_type.flags |= FT_LOAD_NO_HINTING;
		break;
	case 2:
		font_type.flags |= FT_LOAD_FORCE_AUTOHINT;
		break;
	}

		//�����������hinting������defaultģʽ������ʹ��autohintģʽ���Ա�֤Ч��
		// �A���`�G�C���A�X
	if (FTInfo.IsMono()) {
		font_type.flags |= FT_LOAD_TARGET_MONO;
		render_mode = FT_RENDER_MODE_MONO;
	} else {
		switch (pfs->GetAntiAliasMode()) {
		case -1:
			font_type.flags |= FT_LOAD_TARGET_MONO;
			render_mode = FT_RENDER_MODE_MONO;
			break;
		case 0:
			font_type.flags |= FT_LOAD_TARGET_NORMAL;
			render_mode = FT_RENDER_MODE_NORMAL;
			break;
		case 1:
			font_type.flags |= FT_LOAD_TARGET_LIGHT;
			render_mode = FT_RENDER_MODE_LIGHT;
			break;
		case 2:
		case 3:
			font_type.flags |= FT_LOAD_TARGET_LCD;
			render_mode = FT_RENDER_MODE_LCD;
			break;
		case 4:
		case 5:
			font_type.flags |= FT_LOAD_TARGET_LIGHT;
			render_mode = FT_RENDER_MODE_LCD;
			break;
		}
	}

	if (pSettings->HintSmallFont() && font_type.flags & FT_LOAD_TARGET_LIGHT && font_type.height!=-1 && font_type.height<12)  //ͨ�����ò�ʹ��hinting�����Ǵ���С����hinting����
	{	
		/*
		if (!(freetype_face->face_flags & FT_FACE_FLAG_TRICKY))	//�������tricky����
					font_type.flags = font_type.flags & (~FT_LOAD_NO_HINTING) | (pfi->FontHasHinting() ? FT_LOAD_NO_AUTOHINT : FT_LOAD_FORCE_AUTOHINT);
				else*/
		
			font_type.flags = font_type.flags & (~FT_LOAD_NO_HINTING)/* | (pfi->FontHasHinting() ? FT_LOAD_DEFAULT : FT_LOAD_FORCE_AUTOHINT)*/; 
	}

	FTInfo.useKerning = FALSE;
	if (pfs->GetKerning()) {
		switch (pSettings->FontLoader()) {
		case SETTING_FONTLOADER_FREETYPE:
			FTInfo.useKerning = !!FT_HAS_KERNING(freetype_face);
			break;
		case SETTING_FONTLOADER_WIN32:
			{
				DWORD rc = GetFontLanguageInfo(FTInfo.hdc);
				if (rc != GCP_ERROR) {
					FTInfo.useKerning = !!(rc & GCP_USEKERNING);
					FTInfo.ggokerning.init(FTInfo.hdc);
				}
			}
			break;

		DEFAULT_UNREACHABLE;
		}
	}
	return TRUE;
}

// �c�ɂ�����true(ASCII�Ɣ��p�J�i��false)
inline bool IsVerticalChar(WCHAR wch){
	if(wch < 0x80)
		return false;
	if(0xFF61 <= wch && wch <= 0xFF9F)
		return false;
	// �{���͂����Ɛ^�ʖڂɂ��Ȃ��Ƃ܂������B
	return true;
}

struct CGGOFont
{
	HDC m_hdc;
	HFONT m_hfont;
	HFONT m_hprevfont;
	CGGOFont(HDC hdc, const LOGFONT& olf) : m_hdc(hdc), m_hfont(NULL), m_hprevfont(NULL) {
		LOGFONT lf = olf;
		lf.lfWeight = FW_REGULAR;
		lf.lfItalic = FALSE;
		lf.lfStrikeOut = FALSE;
		m_hfont = CreateFontIndirect(&lf);
	}
	~CGGOFont() {
		if (m_hprevfont) {
			SelectFont(m_hdc, m_hprevfont);
		}
		DeleteFont(m_hfont);
	}
	void change() {
		m_hprevfont = SelectFont(m_hdc, m_hfont);
	}
	void restore() {
		SelectFont(m_hdc, m_hprevfont);
		m_hprevfont = NULL;
	}
	operator HFONT () { return m_hfont; }
};

class ClpDx
{
private:
	const INT *p;
	const INT step;
public:
	ClpDx(const INT *lpDx, UINT etoOptions) : p(lpDx), step((etoOptions & ETO_PDY) ? 2 : 1) {
	}
	~ClpDx() {
	}
	int get(int val) {
		int result;
		if (p) {
			result = *p;
			p += step;
		} else {
			result = val;
		}
		return result;
	}
};
/*
FT_UInt FTC_CMapCache_Lookup2( FTC_CMapCache  cache,
					 FTC_FaceID     face_id,
					 FT_Int         cmap_index,
					 FT_UInt32      char_code,
					 FT_Face		freetype_face)
{	
	if ((int)face_id >= charmapCacheSize)
	{
		int oldsize = charmapCacheSize;
		charmapCacheSize = ((int)face_id / 100 + 1)*100;
	    g_charmapCache = (FT_Int*)realloc(g_charmapCache, charmapCacheSize*sizeof(FT_Int));
		memset(&g_charmapCache[oldsize], 0xff, (charmapCacheSize-oldsize)*sizeof(FT_Int));
	}
	if (g_charmapCache[(int)face_id]==-1) 
		if (!FTC_Manager_LookupFace(cache_man, face_id, &freetype_face))
		{
			g_charmapCache[(int)face_id] = FT_Get_Charmap_Index(freetype_face->charmap);
			cmap_index = g_charmapCache[(int)face_id];
		}
		else
			cmap_index = 0;
	return FTC_CMapCache_Lookup(cache, face_id, cmap_index, char_code);
}*/


BOOL ForEachGetGlyphFT(FreeTypeDrawInfo& FTInfo, LPCTSTR lpString, int cbString, FT_Glyph* GlyphArray, FT_DRAW_STATE* drState)
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	//Snowie!!
	BOOL bIsSymbol = GetTextCharsetInfo(FTInfo.hdc, NULL, 0) == SYMBOL_CHARSET;
	BOOL bAllowDefaultLink = pSettings->GetFontLinkInfo().IsAllowFontLink((BYTE)GetTextCharsetInfo(FTInfo.hdc, NULL, 0));	//�Ƿ�Ϊ����
	BOOL nRet = true;
	BOOL bWindowsLink = pSettings->FontLink()==2;
	//!!Snowie

	/*const*/ FT_Face freetype_face = FTInfo.freetype_face;	//ȥ���������ԣ�����Ҫ����
	const FT_Int cmap_index = FTInfo.cmap_index;
	const FT_Bool useKerning = FTInfo.useKerning;
	FT_Render_Mode render_mode = FTInfo.render_mode;
	const int LinkNum = FTInfo.face_id_list_num;
	int AAMode = FTInfo.pfs->GetAntiAliasMode();
	int* AAList = FTInfo.AAModes;
	const LOGFONTW& lf = FTInfo.LogFont();
	FreeTypeFontCache* pftCache = FTInfo.pftCache;
	const CFontSettings*& pfs = FTInfo.pfs;
	FreeTypeFontInfo*& pfi	= FTInfo.pfi;
	const bool bGlyphIndex = FTInfo.IsGlyphIndex();
	//const bool bSizeOnly = FTInfo.IsSizeOnly();
	//const bool bOwnCache = !(FTInfo.font_type.flags & FT_LOAD_RENDER);
	const LPCTSTR lpStart = lpString;
	const LPCTSTR lpEnd = lpString + cbString;
	FT_UInt previous = 0;
	WCHAR previouswch = 0;
	const bool bVertical = lf.lfFaceName[0] == _T('@');
	bool bLcdMode = render_mode == FT_RENDER_MODE_LCD;
	bool bLightLcdMode = (AAMode == 4) || (AAMode == 5);
	ClpDx clpdx(FTInfo.lpDx, FTInfo.params->etoOptions);
	const bool bWidthGDI32 = true;
	const int ggoformatbase = (FTInfo.font_type.flags & FT_LOAD_NO_HINTING) ? GGO_UNHINTED | GGO_NATIVE : GGO_NATIVE;

	if (!s_GGOGlyphLoader.init(freetype_library)) {
		return FALSE;
	}
	

	WORD * gi = new WORD[cbString];
	WORD * ggi=gi;


//Snowie!!

//Fast fontlink
	WORD ** lpfontlink = NULL;
	HFONT hOldFont = NULL;
	if (!bGlyphIndex && bWindowsLink)	//ʹ��Windows fontlink
	{
		lpfontlink = (WORD**)new LPVOID[FTInfo.face_id_list_num];
		for (int i=0;i<LinkNum;i++)
		{
			lpfontlink[i] = new WORD[cbString];
			ZeroMemory(lpfontlink[i], sizeof(WORD)*cbString);	//��ʼ��Ϊ������
		}
		//
		hOldFont = (HFONT)GetCurrentObject(FTInfo.hdc, OBJ_FONT);	//���ص�һ������
	}
//fontlink

	int* Dx= FTInfo.Dx;
	if (!bAllowDefaultLink && FTInfo.face_id_list_num > 1)
		FTInfo.face_id_list_num--;	//�����symbolҳ�ǾͲ����ӵ�����

	bool bUnicodePlane = false;
	for (int i=0 ; lpString < lpEnd; ++lpString, ++gi, ++GlyphArray, ++drState, ++AAList, /*ggdi32++,*/ i++){
		if (bUnicodePlane)
		{
			*drState = FT_DRAW_NOTFOUND;
			bUnicodePlane = false;
			clpdx.get(0);
			FTInfo.px = FTInfo.x;
			goto cont;
		}
		WCHAR wch = *lpString;
		if (!bGlyphIndex && bIsSymbol && !bWindowsLink)
			wch |= 0xF000;
		FT_Glyph* glyph_bitmap = GlyphArray;
		int gdi32x = 0;// = *ggdi32;
		FTInfo.font_type.face_id = FTInfo.face_id_list[0];
		FreeTypeCharData* chData = NULL;
		FT_UInt glyph_index = 0;
		BOOL bIsBold = false, bIsIndivBold = false;

		{
			
			chData = bGlyphIndex
				? pftCache->FindGlyphIndex(wch)
				: pftCache->FindChar(wch);

			if (chData/* && FTInfo.width==chData->GetWidth()*/) {

				gdi32x = chData->GetGDIWidth();
				*AAList = chData->GetAAMode();
				CCriticalSectionLock __lock(CCriticalSectionLock::CS_LIBRARY);
				FT_Glyph_Copy((FT_Glyph)chData->GetGlyph(render_mode), glyph_bitmap);
				//TRACE(_T("Cache Hit: %wc, size:%d, 0x%8.8X\n"), wch, chData->GetWidth(), glyph_bitmap);
			}
		}
		if (!*glyph_bitmap) {
			FT_Glyph glyph = NULL;
			bool f_glyph = false;
			//GLYPHMETRICS gm;
			const MAT2 mat2 = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
			UINT ggoformat = ggoformatbase;
			CTempMem<PVOID> ggobuf;
			DWORD outlinesize = 0;

			if (bGlyphIndex) {
				f_glyph = !!wch;
				glyph_index = wch;
				*AAList = AAMode;
				GetCharWidthI(FTInfo.hdc, wch, 1, (LPWORD)&wch, &gdi32x);	//index�����ֱ���������
				if (FTInfo.font_type.height<=pSettings->BitmapHeight() && pfi->EmbeddedBmpExist(FTInfo.font_type.height))
				{
					f_glyph=false;	//ʹ�õ��󣬲���ͼ
					*drState=FT_DRAW_EMBEDDED_BITMAP;	//����Ϊ�����ͼ��ʽ
				}
			} else
			if (wch && !CID.myiswcntrl(lpString[0])) {
				
				for (int j = 0; j < FTInfo.face_id_list_num; ++j) {
					if (bWindowsLink)	//ʹ��Windows��������fontlink
					{
						if (!lpfontlink[j][i])	//��û��ʼ���������fontlink
						{
							SelectFont(FTInfo.hdc, FTInfo.ggo_font_list[j]);	//����ggo����
							GetGlyphIndices(FTInfo.hdc, lpString, cbString-i, &lpfontlink[j][i], GGI_MARK_NONEXISTING_GLYPHS);	//����fontlink
							SelectFont(FTInfo.hdc, hOldFont);
						}
						glyph_index = lpfontlink[j][i];
						if (glyph_index==0xffff)
							glyph_index = 0;
					}
					else		//ʹ��freetype����fontlink
					{
						CCriticalSectionLock __lock(CCriticalSectionLock::CS_MANAGER);
						glyph_index = FTC_CMapCache_Lookup(cmap_cache,FTInfo.face_id_list[j],-1,wch);
					}
					if (glyph_index) {
						GetCharWidth32W(FTInfo.hdc, wch, wch, &gdi32x);	//��Ч���֣��������
						f_glyph = true;
						FTInfo.font_type.face_id = FTInfo.face_id_list[j];
						freetype_face = FTInfo.GetFace(j);	//ͬʱ���¶�Ӧfaceid��ʵ��face
						//���������¶�Ӧ��fontsetting
						FTInfo.font_type.flags = FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;
						// �q���e�B���O
						//extern CFontSetCache g_fsetcache;
						//pfs = g_fsetcache.Get(FTInfo.font_type.face_id);
						if (FTInfo.font_type.face_id==FTInfo.face_id_simsun && j>0)
						{
							switch (FTInfo.font_type.height)
							{
							case 11:{FTInfo.font_type.height=12; FTInfo.font_type.width++; break;}	//������������⴦��
							case 13:{FTInfo.font_type.height=15; FTInfo.font_type.width+=2; break;}
							}
						}
						pfi = g_pFTEngine->FindFont((int)FTInfo.font_type.face_id);
						if (pfi)
						{
							pfs = &pfi->GetFontSettings();
							switch (pfs->GetHintingMode()) {
							case 0:
								// ignore.
								break;
							case 1:
								FTInfo.font_type.flags |= FT_LOAD_NO_HINTING;
								break;
							case 2:
								FTInfo.font_type.flags |= FT_LOAD_FORCE_AUTOHINT;
								break;
							}
							// �A���`�G�C���A�X
							if (FTInfo.IsMono()) {
								FTInfo.font_type.flags |= FT_LOAD_TARGET_MONO;
								render_mode = FT_RENDER_MODE_MONO;
							} else {
								switch (*AAList = pfs->GetAntiAliasMode()) {
								case -1:
									FTInfo.font_type.flags |= FT_LOAD_TARGET_MONO;
									render_mode = FT_RENDER_MODE_MONO;
									break;
								case 0:
									FTInfo.font_type.flags |= FT_LOAD_TARGET_NORMAL;
									render_mode = FT_RENDER_MODE_NORMAL;
									break;
								case 1:
									FTInfo.font_type.flags |= FT_LOAD_TARGET_LIGHT;
									render_mode = FT_RENDER_MODE_LIGHT;
									break;
								case 2:
								case 3:
									FTInfo.font_type.flags |= FT_LOAD_TARGET_LCD;
									render_mode = FT_RENDER_MODE_LCD;
									break;
								case 4:
								case 5:
									FTInfo.font_type.flags |= FT_LOAD_TARGET_LIGHT;
									render_mode = FT_RENDER_MODE_LCD;
									break;
								}
							}
							if (pSettings->HintSmallFont() && FTInfo.font_type.flags & FT_LOAD_TARGET_LIGHT && FTInfo.font_type.height!=-1 && FTInfo.font_type.height<12)  //ͨ�����ò�ʹ��hinting�����Ǵ���С����hinting����
								FTInfo.font_type.flags = FTInfo.font_type.flags & (~FT_LOAD_NO_HINTING)/* | (pfi->FontHasHinting() ? FT_LOAD_DEFAULT : FT_LOAD_FORCE_AUTOHINT)*/;

							AAMode = *AAList/*pfs->GetAntiAliasMode()*/;
							bLcdMode = render_mode == FT_RENDER_MODE_LCD;
							bLightLcdMode = (AAMode == 4) || (AAMode == 5);
							//�������
						}
						if (FTInfo.font_type.height<=pSettings->BitmapHeight() && pfi->EmbeddedBmpExist(FTInfo.font_type.height))
						{
							f_glyph=false;	//ʹ�õ��󣬲���ͼ
							*drState=FT_DRAW_EMBEDDED_BITMAP;	//����Ϊ�����ͼ��ʽ
						}
						break;
					}
				}
			}

			

			if (!f_glyph) {	//glyphindex�����������Ѿ��������
#ifdef _DEBUG
				GdiSetBatchLimit(0);
#endif
				if (*drState==FT_DRAW_NORMAL || bGlyphIndex)
					*drState = FT_DRAW_NOTFOUND;	//�Ҳ�������
				if ((!FTInfo.lpDx || lpString == lpEnd - 1) && !bGlyphIndex)	//��Ч���֣�����û�������Ű�������Ű�����һ���ַ���
				{
					GetCharWidth32W(FTInfo.hdc, wch, wch, &gdi32x);
				}
				int cx = gdi32x;
/*
				if (bSizeOnly) {
					FTInfo.x += cx;
				} else*/
				
				{
					if (wch) {
						*glyph_bitmap = NULL;	//��Ч����
						//ORIG_ExtTextOutW(FTInfo.hdc, FTInfo.x, FTInfo.yTop, FTInfo.GetETO(), NULL, &wch, 1, NULL);
					}
					BOOL isc = bGlyphIndex ? false : (CID.myiswcntrl(*lpString));
					if (isc == CNTRL_UNICODE_PLANE)
						bUnicodePlane = true;
// 					else
// 						if (isc == CNTRL_ZERO_WIDTH)	//Ԥ������޿��ȿ�����
// 							cx = 0;
					int dxWidth = clpdx.get(cx);
					if (isc == CNTRL_COMPLEX_TEXT)	//������
					{
						cx = dxWidth;	//����windows�Ŀ��ȵ���
						//if (!dxWidth)
						//	CID.setcntrlAttribute(wch, CNTRL_ZERO_WIDTH);
					}
					if (lpString < lpEnd - 1) {
						FTInfo.x += dxWidth;
					} else {
						//if (gdi32x)
						//{
							
	/*						ABC abc = {0, cx, 0};
							if (bGlyphIndex)
								GetCharABCWidthsI(FTInfo.hdc, wch, 1, NULL, &abc);
							else
								GetCharABCWidths(FTInfo.hdc, wch, wch, &abc);*/
							//FTInfo.px = FTInfo.x+Max(clpdx.get(cx), abc.abcA+(int)abc.abcB+abc.abcC);	//��Ч���ֵ�����£���ͼ����=���λ��
							FTInfo.px = FTInfo.x + cx;	
							FTInfo.x += dxWidth;//Max(clpdx.get(cx), cx);/*(int)abc.abcB+abc.abcC*///Max(clpdx.get(cx), abc.abcB? abc.abcA:0);
						//}
					}
				if (!isc)
					FTInfo.x += FTInfo.params->charExtra;
				}
				goto cont;
			}

				// �c����
				if(bVertical){
					glyph_index = ft2vert_get_gid(
						(struct ft2vert_st *)freetype_face->generic.data,
						glyph_index);
				}

				// �J�[�j���O
				if(useKerning){
					if(previous != 0 && glyph_index){
						FT_Vector delta;
						FT_Get_Kerning(freetype_face,
									   previous, glyph_index,
									   ft_kerning_default, &delta);
						FTInfo.x += FT_PosToInt(delta.x);
					}
					previous = glyph_index;
				}


			// �c��
			if(bVertical && IsVerticalChar(wch)){
				FTInfo.font_type.flags |= FT_LOAD_VERTICAL_LAYOUT;
				if(bLcdMode){
					if(!bLightLcdMode){
						FTInfo.font_type.flags &= ~FT_LOAD_TARGET_LCD;
						FTInfo.font_type.flags |= FT_LOAD_TARGET_LCD_V;
					}
					render_mode             = FT_RENDER_MODE_LCD_V;
				}
			}else{
				if (bVertical)
					swap(FTInfo.font_type.height, FTInfo.font_type.width);	//�����޷���ת�����ֿ���
				FTInfo.font_type.flags &=~FT_LOAD_VERTICAL_LAYOUT;
				if(bLcdMode){
					if(!bLightLcdMode){
						FTInfo.font_type.flags &= ~FT_LOAD_TARGET_LCD_V;
						FTInfo.font_type.flags |= FT_LOAD_TARGET_LCD;
					}
					render_mode             = FT_RENDER_MODE_LCD;
				}
			}

		{
			
			bool bRequiredownsize;

				bIsIndivBold = freetype_face->style_flags & FT_STYLE_FLAG_BOLD;	//�Ƕ�������
				bIsBold = (IsFontBold(lf) && !bIsIndivBold);	//�Ƿ´���
				bRequiredownsize = bIsBold && /*(pSettings->BolderMode()==2 || (*/pSettings->BolderMode()!=1 /*&& FTInfo.height>FT_BOLD_LOW))*/;
				if (bRequiredownsize)
				{				
					FTInfo.font_type.width -= (FTInfo.font_type.width)/36;
					FTInfo.font_type.height -= (FTInfo.font_type.height)/36;
				}

				{
					CCriticalSectionLock __lock(CCriticalSectionLock::CS_MANAGER);
					if(FTC_ImageCache_Lookup(
						image_cache,
						&FTInfo.font_type,
						glyph_index,
						&glyph,
						NULL)) {
							nRet= false;
							goto gdiexit;
					}
					//glyph = New_FT_Glyph();
				}

				FTInfo.font_type.height = FTInfo.height;
				FTInfo.font_type.width = FTInfo.width;
				
		}
		{
			CCriticalSectionLock __lock(CCriticalSectionLock::CS_LIBRARY);
			if (FT_Glyph_Copy(glyph, glyph_bitmap))
			{	
				//FT_Done_Glyph(glyph);
				nRet = FALSE;
				goto gdiexit;
			}
			//FT_Done_Glyph(glyph);
		}
			if((*glyph_bitmap)->format != FT_GLYPH_FORMAT_BITMAP){
				int str_h;
				int str_v;
				bool fbold = false;
				str_h = str_v = FTInfo.pfi->CalcNormalWeight();
				if (bIsIndivBold)
					str_h = str_v = FTInfo.pfi->GetExactBoldWeight()<<2;
				if (bIsBold) {
					fbold = true;
					str_h += FTInfo.font_type.height<24 ? FTInfo.pfi->GetFTWeight(): (FTInfo.pfi->GetFTWeight()*FTInfo.font_type.height/24);
					str_v = str_h;
				}
				if((str_h || str_v) && New_FT_Outline_Embolden(
					   &((FT_OutlineGlyph)(*glyph_bitmap))->outline,
					   str_h, str_v, FTInfo.height))
					{
						FT_Done_Glyph(*glyph_bitmap);
						nRet= false;
						goto gdiexit;
					}

				if (fbold) {
					((FT_BitmapGlyph)(*glyph_bitmap))->root.advance.x += 0x10000;
				}
				if(lf.lfItalic &&
					!(freetype_face->style_flags & FT_STYLE_FLAG_ITALIC)){
					FT_Matrix matrix;
					FTInfo.pfi->CalcItalicSlant(matrix);
					FT_Outline_Transform(
						&((FT_OutlineGlyph)((*glyph_bitmap)))->outline,
						&matrix);
				}
				{
					CCriticalSectionLock __lock(CCriticalSectionLock::CS_LIBRARY);
					if(FT_Glyph_To_Bitmap(&((*glyph_bitmap)), render_mode, 0, 1)){
						FT_Done_Glyph(*glyph_bitmap);
						nRet= false;
						goto gdiexit;
					}
				}
			}
		}

		int cx = (bVertical && IsVerticalChar(wch)) ?
				FT_FixedToInt(FT_BitmapGlyph((*glyph_bitmap))->root.advance.y) :
				FT_FixedToInt(FT_BitmapGlyph((*glyph_bitmap))->root.advance.x);

{
			int dx = clpdx.get(bWidthGDI32 ? gdi32x : cx);	//��ÿ���
			int left = FT_BitmapGlyph((*glyph_bitmap))->left;
			if (FTInfo.x + left< FTInfo.xBase)
				FTInfo.xBase = FTInfo.x + left;	//������ַ��Ǹ�����ʼλ�õģ��ϳɷ��ţ��� �������ֵ���ʼλ��

			if (lpString < lpEnd - 1) {
				FTInfo.x += dx;
			} else {
					int bx = FT_BitmapGlyph((*glyph_bitmap))->bitmap.width;
					if (render_mode == FT_RENDER_MODE_LCD) bx /= 3;
					bx += left;
					FTInfo.px = FTInfo.x + Max(Max(dx, bx), cx);	//�����ֵ������,��ͼ����=ft����Ŀ��ȣ����λ��=win����
					FTInfo.x += dx;//Max(dx, gdi32x);//Max(Max(dx, bx), cx);
			}

		}
		FTInfo.x += FTInfo.params->charExtra;

		//if (bSizeOnly || bOwnCache) {
			//�L���b�V����
		if (glyph_index) {
			
			if (bGlyphIndex) {
				pftCache->AddGlyphData(glyph_index, /*cx*/FTInfo.width, gdi32x, (FT_BitmapGlyph)*glyph_bitmap, render_mode, AAMode);
			} else {
				pftCache->AddCharData(wch, glyph_index, /*cx*/FTInfo.width, gdi32x, (FT_BitmapGlyph)*glyph_bitmap, render_mode, AAMode);
			}
		}

cont:
		*Dx = FTInfo.x;		//Dx��λ������һ���ַ���ʼ�Ļ�׼λ�ã���������һ���ַ���ʼ����λ��
		++Dx;
	}
gdiexit:
	delete[] ggi;
//	delete[] gdi32w;

	if (!bGlyphIndex && bWindowsLink)
	{
		for (int i=0;i<LinkNum;i++)
			delete lpfontlink[i];
		delete lpfontlink;
	}
	return nRet;
}


BOOL ForEachGetGlyphGGO(FreeTypeDrawInfo& FTInfo, LPCTSTR lpString, int cbString, FT_Glyph* GlyphArray, FT_DRAW_STATE* drState)
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	//Snowie!!
	BOOL bIsSymbol = GetTextCharsetInfo(FTInfo.hdc, NULL, 0) == SYMBOL_CHARSET;
	BOOL bAllowDefaultLink = pSettings->GetFontLinkInfo().IsAllowFontLink((BYTE)GetTextCharsetInfo(FTInfo.hdc, NULL, 0));	//�Ƿ�Ϊ����
	BOOL nRet = true;
	BOOL bWindowsLink = pSettings->FontLink()==2;
	//!!Snowie

	/*const*/ FT_Face freetype_face = FTInfo.freetype_face;	//ȥ���������ԣ�����Ҫ����
	const FT_Int cmap_index = FTInfo.cmap_index;
	const FT_Bool useKerning = FTInfo.useKerning;
	FT_Render_Mode render_mode = FTInfo.render_mode;
	const int LinkNum = FTInfo.face_id_list_num;
	int AAMode = FTInfo.pfs->GetAntiAliasMode();
	int* AAList = FTInfo.AAModes;
	const LOGFONTW& lf = FTInfo.LogFont();
	FreeTypeFontCache* pftCache = FTInfo.pftCache;
	const CFontSettings*& pfs = FTInfo.pfs;
	FreeTypeFontInfo*& pfi	= FTInfo.pfi;
	const bool bGlyphIndex = FTInfo.IsGlyphIndex();
	//const bool bSizeOnly = FTInfo.IsSizeOnly();
	//const bool bOwnCache = !(FTInfo.font_type.flags & FT_LOAD_RENDER);
	const LPCTSTR lpStart = lpString;
	const LPCTSTR lpEnd = lpString + cbString;
	FT_UInt previous = 0;
	WCHAR previouswch = 0;
	const bool bVertical = false;
	bool bLcdMode = render_mode == FT_RENDER_MODE_LCD;
	bool bLightLcdMode = (AAMode == 4) || (AAMode == 5);
	ClpDx clpdx(FTInfo.lpDx, FTInfo.params->etoOptions);
	const bool bWidthGDI32 = pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32;
	const int ggoformatbase = (FTInfo.font_type.flags & FT_LOAD_NO_HINTING) ? GGO_UNHINTED | GGO_NATIVE : GGO_NATIVE;

	if (!s_GGOGlyphLoader.init(freetype_library)) {
		return FALSE;
	}
// 	LPCTSTR dumy = lpString;
// 	if (!bGlyphIndex)
// 	 for (; dumy<lpEnd;dumy++)
// 		if (iswcntrl(*dumy))
// 		{
// 			return false;
// 		}		

	WORD * gi = new WORD[cbString];
	WORD * ggi=gi;
	//int* gdi32w = new int[cbString];
	//int* ggdi32 = gdi32w;
	//SIZE* szSize =new SIZE[cbString];
	//SIZE* sSize = szSize;

//Snowie!!

//Fast fontlink
	WORD ** lpfontlink = NULL;
	HFONT hOldFont = NULL;
	if (!bGlyphIndex && bWindowsLink)	//ʹ��Windows fontlink
	{
		lpfontlink = (WORD**)new LPVOID[FTInfo.face_id_list_num];
		for (int i=0;i<LinkNum;i++)
		{
			lpfontlink[i] = new WORD[cbString];
			ZeroMemory(lpfontlink[i], sizeof(WORD)*cbString);	//��ʼ��Ϊ������
		}
		//
		hOldFont = (HFONT)GetCurrentObject(FTInfo.hdc, OBJ_FONT);	//���ص�һ������
	}
//fontlink

	/*
	if (!FTInfo.lpDx)	//û��Ԥ�ȼ����Ű棬��Ҫ���ÿ�����ֵĿ�����Ϣ
		{
			if (bGlyphIndex)
			{
				(GetCharWidthI(FTInfo.hdc, *(LPWORD)lpString, cbString, (LPWORD)lpString, gdi32w));
			}
			else
			{
				for (int i=0;i<cbString;i++, ggdi32++)
					GetCharWidth32W(FTInfo.hdc, lpString[i], lpString[i], ggdi32);
				ggdi32 = gdi32w;
			}
		}
		else
		{
			//Ԥ�ȼ�������Ű棬ֻ��Ҫ������һ���ֵ���Ϣ�Ϳ�����
			if (bGlyphIndex)
			{
				(GetCharWidthI(FTInfo.hdc, *(((LPWORD)lpString)+cbString-1), 1, (((LPWORD)lpString)+cbString-1), gdi32w+cbString-1));
			}
			else
			{
				GetCharWidth32W(FTInfo.hdc, lpString[cbString-1], lpString[cbString-1], ggdi32+cbString-1);
				ggdi32 = gdi32w;
			}
		}*/
	
	if (!bGlyphIndex)  	//����win32��������Ż���ft�������
		if (GetGlyphIndices(FTInfo.hdc, lpString, cbString, gi, GGI_MARK_NONEXISTING_GLYPHS)!=cbString) 
		{
			nRet = false;
			goto gdiexit;
		}
//!!Snowie
	int* Dx= FTInfo.Dx;
	if (!bAllowDefaultLink && FTInfo.face_id_list_num > 1)
		FTInfo.face_id_list_num--;	//�����symbolҳ�ǾͲ����ӵ�����

	for (int i=0 ; lpString < lpEnd; ++lpString, gi++, GlyphArray++, drState++, ++AAList,/*ggdi32++,*/ i++){
		WCHAR wch = *lpString;
		if (!bGlyphIndex && bIsSymbol && !bWindowsLink)
			wch |= 0xF000;
		FT_Glyph* glyph_bitmap = GlyphArray;
		int gdi32x = 0;// = *ggdi32;
		FTInfo.font_type.face_id = FTInfo.face_id_list[0];
		FreeTypeCharData* chData = NULL;
		FT_UInt glyph_index = 0;
		BOOL bIsBold = false, bIsIndivBold = false;

		{
			
			chData = bGlyphIndex
				? pftCache->FindGlyphIndex(wch)
				: pftCache->FindChar(wch);

			if (chData && FTInfo.width==chData->GetWidth()) {
/*
				if (bSizeOnly) {
					//TRACE(_T("Cache hit: GetCharWidth [%c]\n"), *lpString);
					int cx = chData->GetWidth();
					FTInfo.x += (bWidthGDI32 ? gdi32x : cx) + FTInfo.params->charExtra;
					goto cont;
				}*/
				gdi32x = chData->GetGDIWidth();
				*AAList = chData->GetAAMode();
				CCriticalSectionLock __lock(CCriticalSectionLock::CS_LIBRARY);
				FT_Glyph_Copy((FT_Glyph)chData->GetGlyph(render_mode), glyph_bitmap);
				//TRACE(_T("Cache Hit: %wc, size:%d, 0x%8.8X\n"), wch, chData->GetWidth(), glyph_bitmap);
			}
		}
		if (!*glyph_bitmap) {
			FT_Glyph glyph = NULL;
			bool f_glyph = false;
			GLYPHMETRICS gm;
			const MAT2 mat2 = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
			UINT ggoformat = ggoformatbase;
			CTempMem<PVOID> ggobuf;
			DWORD outlinesize = 0;

			
				if (bGlyphIndex) {
					f_glyph = true;
					*AAList = AAMode;
					glyph_index = wch;
					ggoformat |= GGO_GLYPH_INDEX;
					GetCharWidthI(FTInfo.hdc, wch, 1, (LPWORD)&wch, &gdi32x);	//index�����ֱ���������
				} else
				{
					if (*(gi) != 0xffff) {
							glyph_index = *(gi);
							f_glyph = true;
							*AAList = AAMode;
						}
					GetCharWidth32W(FTInfo.hdc, wch, wch, &gdi32x);	//��Ч���֣��������
				}
				if (lpString == lpStart && FTInfo.font_type.flags & FT_LOAD_FORCE_AUTOHINT) {
					// FORCE_AUTOHINT 
					GetGlyphOutlineW(FTInfo.hdc, 0, GGO_METRICS | GGO_GLYPH_INDEX | GGO_NATIVE | GGO_UNHINTED, &gm, 0, NULL, &mat2);
				}
				outlinesize = GetGlyphOutlineW(FTInfo.hdc, wch, ggoformat, &gm, ggobuf.getsize(), ggobuf, &mat2);

				if (outlinesize == GDI_ERROR || outlinesize == 0){
					glyph_index = 0;
					f_glyph = false;
				} else 
				{
					glyph_index = wch;
					f_glyph = true;
				}


			if (!f_glyph) {	//glyphindex�����������Ѿ��������
#ifdef _DEBUG
				GdiSetBatchLimit(0);
#endif
				if (*drState==FT_DRAW_NORMAL || bGlyphIndex)
					*drState = FT_DRAW_NOTFOUND;	//�Ҳ�������
				if ((!FTInfo.lpDx || lpString == lpEnd - 1) && !bGlyphIndex)	//��Ч���֣�����û�������Ű�������Ű�����һ���ַ���
				{
					GetCharWidth32W(FTInfo.hdc, wch, wch, &gdi32x);
				}
				int cx = gdi32x;
/*
				if (bSizeOnly) {
					FTInfo.x += cx;
				} else*/
				
				{
					if (wch) {
						*glyph_bitmap = NULL;	//��Ч����
						//ORIG_ExtTextOutW(FTInfo.hdc, FTInfo.x, FTInfo.yTop, FTInfo.GetETO(), NULL, &wch, 1, NULL);
					}
					BOOL isc = bGlyphIndex ? false : (CID.myiswcntrl(*lpString));
					if (isc)
						cx = 0;
					if (lpString < lpEnd - 1) {
						FTInfo.x += clpdx.get(cx);
					} else {
						//if (gdi32x)
						{
							
	/*						ABC abc = {0, cx, 0};
							if (bGlyphIndex)
								GetCharABCWidthsI(FTInfo.hdc, wch, 1, NULL, &abc);
							else
								GetCharABCWidths(FTInfo.hdc, wch, wch, &abc);*/
							//FTInfo.px = FTInfo.x+Max(clpdx.get(cx), abc.abcA+(int)abc.abcB+abc.abcC);	//��Ч���ֵ�����£���ͼ����=���λ��
							FTInfo.px = FTInfo.x + cx;	
							FTInfo.x += clpdx.get(cx);
						}
					}
				if (!isc)
					FTInfo.x += FTInfo.params->charExtra;
				}
				goto cont;
			}

			
				if(useKerning && !bGlyphIndex){
					if (previouswch && wch) {
						FTInfo.x += FTInfo.ggokerning.get(previouswch, wch);
					}
					previouswch = wch;
				}


			// �c��
			if(bVertical && IsVerticalChar(wch)){
				FTInfo.font_type.flags |= FT_LOAD_VERTICAL_LAYOUT;
				if(bLcdMode){
					if(!bLightLcdMode){
						FTInfo.font_type.flags &= ~FT_LOAD_TARGET_LCD;
						FTInfo.font_type.flags |= FT_LOAD_TARGET_LCD_V;
					}
					render_mode             = FT_RENDER_MODE_LCD_V;
				}
			}else{
				if (bVertical)
					swap(FTInfo.font_type.height, FTInfo.font_type.width);	//�����޷���ת�����ֿ���
				FTInfo.font_type.flags &=~FT_LOAD_VERTICAL_LAYOUT;
				if(bLcdMode){
					if(!bLightLcdMode){
						FTInfo.font_type.flags &= ~FT_LOAD_TARGET_LCD_V;
						FTInfo.font_type.flags |= FT_LOAD_TARGET_LCD;
					}
					render_mode             = FT_RENDER_MODE_LCD;
				}
			}

			CGGOOutlineGlyph ggoog;
		{
			
				if (outlinesize > ggobuf.getsize()) {
					if (!ggobuf.init(outlinesize)) {
						nRet= false;
						goto gdiexit;
					}
					//ggofont.change();
					outlinesize = GetGlyphOutlineW(FTInfo.hdc, wch, ggoformat, &gm, ggobuf.getsize(), ggobuf, &mat2);
					//ggofont.restore();
				}
				if (outlinesize > ggobuf.getsize()) {
					nRet= false;
					goto gdiexit;
				}
				if (!ggoog.init(outlinesize, ggobuf, gm)) {
					nRet= false;
					goto gdiexit;
				}
				//glyph = New_FT_Ref_Glyph();
				//FT_Glyph_Copy((FT_Glyph)ggoog, &glyph);
				glyph = ggoog;
		}
		{
			CCriticalSectionLock __lock(CCriticalSectionLock::CS_LIBRARY);
			if (FT_Glyph_Copy(glyph, glyph_bitmap))
			{	
				//FT_Done_Glyph(glyph);
				nRet = FALSE;
				goto gdiexit;
			}
			//FT_Done_Glyph(glyph);
		}
			if((*glyph_bitmap)->format != FT_GLYPH_FORMAT_BITMAP){
				int str_h;
				int str_v;
				bool fbold = false;
				str_h = str_v = FTInfo.pfi->CalcNormalWeight();
				if (bIsIndivBold)
					str_h = str_v = FTInfo.pfi->GetExactBoldWeight()<<2;
				if (bIsBold) {
					fbold = true;
					str_h += FTInfo.font_type.height<24 ? FTInfo.pfi->GetFTWeight(): (FTInfo.pfi->GetFTWeight()*FTInfo.font_type.height/24);
					str_v = str_h;
				}
				if((str_h || str_v) && New_FT_Outline_Embolden(
					   &((FT_OutlineGlyph)((*glyph_bitmap)))->outline,
					   str_h, str_v, FTInfo.height))
					{
						FT_Done_Glyph(*glyph_bitmap);
						nRet= false;
						goto gdiexit;
					}

				if (fbold) {
					((FT_BitmapGlyph)((*glyph_bitmap)))->root.advance.x += 0x10000;
				}

				{
					CCriticalSectionLock __lock(CCriticalSectionLock::CS_LIBRARY);
					if(FT_Glyph_To_Bitmap(&((*glyph_bitmap)), render_mode, 0, 1)){
						FT_Done_Glyph(*glyph_bitmap);
						nRet= false;
						goto gdiexit;
					}
				}
			}
		}

		int cx = (bVertical && IsVerticalChar(wch)) ?
				FT_FixedToInt(FT_BitmapGlyph((*glyph_bitmap))->root.advance.y) :
				FT_FixedToInt(FT_BitmapGlyph((*glyph_bitmap))->root.advance.x);
//done
/*
		if (bSizeOnly) {
			FTInfo.x += bWidthGDI32 ? gdi32x : cx;
		} else */
		{
			int dx = clpdx.get(bWidthGDI32 ? gdi32x : cx);	//��ÿ���
			int left = FT_BitmapGlyph((*glyph_bitmap))->left;
			if (FTInfo.x + left< FTInfo.xBase)
				FTInfo.xBase = FTInfo.x + left;	//������ַ��Ǹ�����ʼλ�õģ��ϳɷ��ţ��� �������ֵ���ʼλ��

			if (lpString < lpEnd - 1) {
				FTInfo.x += dx;
			} else {
				int bx = FT_BitmapGlyph((*glyph_bitmap))->bitmap.width;
				if (render_mode == FT_RENDER_MODE_LCD) bx /= 3;
				bx += left;
				FTInfo.px = FTInfo.x + Max(Max(dx, bx), cx);	//�����ֵ������,��ͼ����=ft����Ŀ��ȣ����λ��=win����
				FTInfo.x += dx;//Max(dx, gdi32x);//Max(Max(dx, bx), cx);
			}

		}
		FTInfo.x += FTInfo.params->charExtra;

		//if (bSizeOnly || bOwnCache) {
			//�L���b�V����
		if (glyph_index) {
			
			if (bGlyphIndex) {
				pftCache->AddGlyphData(glyph_index, /*cx*/FTInfo.width, gdi32x, (FT_BitmapGlyph)*glyph_bitmap, render_mode, AAMode);
			} else {
				pftCache->AddCharData(wch, glyph_index, /*cx*/FTInfo.width, gdi32x, (FT_BitmapGlyph)*glyph_bitmap, render_mode, AAMode);
			}
		}
		//}
// 		if (!bGlyphIndex && iswcntrl(wch) && *glyph_bitmap)
// 		{
// 			
// 			FT_Done_Glyph(*glyph_bitmap);
// 			*glyph_bitmap = NULL;
// 		}
cont:
		*Dx = FTInfo.x;
		Dx++;
	}
gdiexit:
	delete[] ggi;
//	delete[] gdi32w;

	if (!bGlyphIndex && bWindowsLink)
	{
		for (int i=0;i<LinkNum;i++)
			delete lpfontlink[i];
		delete lpfontlink;
	}
	return nRet;
}


BOOL GetLogFontFromDC(HDC hdc, LOGFONT& lf)
{
	LOGFONTW lfForce = { 0 };
	HFONT hf = GetCurrentFont(hdc);
	if (!ORIG_GetObjectW(hf, sizeof(LOGFONTW), &lf))
		return FALSE;

	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	//if (pSettings->CopyForceFont(lfForce, lf))
	//	lf = lfForce;

	if(pSettings->LoadOnDemand()) {
		//AddFontToFT(lf.lfFaceName, lf.lfWeight, !!lf.lfItalic);
	}
	return TRUE;
}

BOOL CALLBACK TextOutCallback(FreeTypeGlyphInfo& FTGInfo)
{
	FreeTypeDrawInfo* FTInfo = FTGInfo.FTInfo;
	FT_BitmapGlyph& glyph_bitmap = FTGInfo.FTGlyph;
	const bool bVertical = FTInfo->LogFont().lfFaceName[0] == _T('@');
	int nOldAlpha = FTInfo->params->alpha;

	if (!FTGInfo.FTGlyph->bitmap.buffer) {
		//if (FTInfo->params->alpha == 1) {
// 		if (!(FTInfo->GetETO() & ETO_GLYPH_INDEX) && wch==32)	//�ո�
// 			ORIG_ExtTextOutW(FTInfo->hdc, FTInfo->x, FTInfo->yTop, FTInfo->GetETO() & ETO_IGNORELANGUAGE, NULL, &wch, 1, NULL);
// 		else
			ORIG_ExtTextOutW(FTInfo->hdc, FTInfo->x, FTInfo->yTop, FTInfo->GetETO(), NULL, &FTGInfo.wch, 1, NULL);
		//}
	} else {
		
		const CGdippSettings* pSettings = CGdippSettings::GetInstance();
		if (bVertical && IsVerticalChar(FTGInfo.wch) &&
			pSettings->FontLoader() == SETTING_FONTLOADER_FREETYPE) {
			if (FTInfo->params->alpha>1)
			{
				FreeTypeDrawBitmapV(FTGInfo, *FTGInfo.shadow, FTInfo->x + FTInfo->sx, 
					FTInfo->params->otm->otmTextMetrics.tmHeight - (glyph_bitmap->left+glyph_bitmap->bitmap.width) -1 + FTInfo->sy);//����Ӱ
				FTInfo->params->alpha = 1;
			}
			FreeTypeDrawBitmapV(FTGInfo,*FTGInfo.solid,	FTInfo->x,
				FTInfo->params->otm->otmTextMetrics.tmHeight - (glyph_bitmap->left+glyph_bitmap->bitmap.width) -1);	//������	
		}else{
			if (FTInfo->params->alpha>1)
			{
				FreeTypeDrawBitmap(FTGInfo,*FTGInfo.shadow,
					FTInfo->x + glyph_bitmap->left + FTInfo->sx,
					FTInfo->yBase - glyph_bitmap->top + FTInfo->sy);	//����Ӱ
				FTInfo->params->alpha = 1;
			}
			FreeTypeDrawBitmap(FTGInfo,*FTGInfo.solid,
				FTInfo->x + glyph_bitmap->left,
				FTInfo->yBase - glyph_bitmap->top);	//������

		}
	}
	FTInfo->params->alpha = nOldAlpha;
	return TRUE;
}

int IsColorDark(DWORD Color, double Gamma) 
{
	//return (GetRValue(Color)*0.299 + GetGValue(Color)*0.587 + GetBValue(Color)*0.114);	//ԭʼ�㷨
	//===============================================================
	//����Photoshop sRGB��RGB->Lab�㷨���л��㣬LΪɫ���Ӿ�����
	//��л ����������ѧ ������ �ķ���
	//===============================================================
	static double s_multipler = 116/pow(100,(double)1.0/3.0);	//Ԥ���㳣��,ǿ��ʹ��double�汾
	double* RGBTable = s_AlphaBlendTable.GetRGBTable();	//�����ʾ��ת����
	double ret = pow(23.9746*RGBTable[GetRValue(Color)] + 73.0653*RGBTable[GetGValue(Color)] + 6.13799*RGBTable[GetBValue(Color)] , 1.0/3.0)*s_multipler-16;
	return max(int(ret + 0.499),0);
	
	/*double r = GetRValue(Color)/255.0;
	double g = GetGValue(Color)/255.0;
	double b = GetBValue(Color)/255.0;
	double v;
	double m;
	double vm;
	double r2, g2, b2;

	double h = 0; // default to black
	double s = 0;
	double l = 0;
	v = Max(r,g);
	v = Max(v,b);
	m = Min(r,g);
	m = Min(m,b);
	l = (m + v) / 2.0;
	if (l <= 0.0)
	{
		return 0;
	}
	vm = v - m;
	s = vm;
	if (s > 0.0)
	{
		s /= (l <= 0.5) ? (v + m ) : (2.0 - v - m) ;
	}
	else
	{
		return l;
	}
	r2 = (v - r) / vm;
	g2 = (v - g) / vm;
	b2 = (v - b) / vm;
	if (r == v)
	{
		h = (g == m ? 5.0 + b2 : 1.0 - g2);
	}
	else if (g == v)
	{
		h = (b == m ? 1.0 + r2 : 3.0 - b2);
	}
	else
	{
		h = (r == m ? 3.0 + g2 : 5.0 - r2);
	}
	h /= 6.0;
	return l;*/
}

/*
BOOL GetColorDiff(DWORD Color)
{
	/ *const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	DWORD ShadowColorD = pSettings->ShadowDarkColor();
	DWORD ShadowColorL = pSettings->ShadowLightColor();
	DWORD ColorDiffD = RGBA(abs(GetRValue(Color)-GetRValue(ShadowColorD)),abs(GetGValue(Color)-GetGValue(ShadowColorD)),abs(GetBValue(Color)-GetBValue(ShadowColorD)),0);
	DWORD ColorDiffL = RGBA(abs(GetRValue(Color)-GetRValue(ShadowColorL)),abs(GetGValue(Color)-GetGValue(ShadowColorL)),abs(GetBValue(Color)-GetBValue(ShadowColorL)),0);
	double cd = IsColorDark(ColorDiffD), cl = IsColorDark(ColorDiffL);
	return cd==cl ? IsColorDark(Color)<0.7 : cd>cl;* /
}*/

BOOL FreeTypeTextOut(
	const HDC hdc,     // �f�o�C�X�R���e�L�X�g�̃n���h��
	CBitmapCache& cache,
	LPCWSTR lpString,  // ������
	int cbString,      // ������
	FreeTypeDrawInfo& FTInfo,
	FT_Glyph * Glyphs,
	FT_DRAW_STATE* drState
	)
{
	if(cbString <= 0 || lpString == NULL)
		return FALSE;
	CAlphaBlendColor * solid = NULL;
	CAlphaBlendColor * shadow = NULL;
	
	//CCriticalSectionLock __lock;

	FT_Face freetype_face = FTInfo.freetype_face;
	const LOGFONT& lf = FTInfo.LogFont();

	FTInfo.x     = -FTInfo.xBase;
	FTInfo.yTop  = 0;

	const TEXTMETRIC& tm = FTInfo.params->otm->otmTextMetrics;
	FTInfo.yBase = tm.tmAscent;

//===============������ɫ����======================

	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	int lightdiff, darkdiff, bDarkColor=0, ShadowColor=0;
	if (FTInfo.params->alpha!=1)
	{
		float Gamma = pSettings->GammaValue();
		bDarkColor = IsColorDark(FTInfo.params->color, Gamma);
		int diff = max(darkdiff=abs(IsColorDark(pSettings->ShadowDarkColor(), Gamma)-bDarkColor), lightdiff=abs(IsColorDark(pSettings->ShadowLightColor(), Gamma)-bDarkColor));
		ShadowColor = lightdiff<=darkdiff ? pSettings->ShadowDarkColor() : pSettings->ShadowLightColor();
		bDarkColor = lightdiff<=darkdiff;
		if (/*diff<10 || abs(lightdiff-darkdiff)<20 &&*/ pSettings->ShadowDarkColor()==pSettings->ShadowLightColor())
		{
		//���ӵ�ɫ���⣬ǿ�ƿ�����Ӱ
			FTInfo.params->alphatuner = 1;
		}
		else
		{
			diff = abs(lightdiff-darkdiff);
			if (diff<10)
				FTInfo.params->alpha = 1;
			else
				FTInfo.params->alphatuner = max(1, 100/diff);	//����ɫ�������ӰŨ��
		}
	}
	char mode = (*Glyphs)? FT_BitmapGlyph((*Glyphs))->bitmap.pixel_mode:FT_PIXEL_MODE_LCD;
	switch(mode){
			case FT_PIXEL_MODE_MONO:
				return false;
				//break;
			case FT_PIXEL_MODE_LCD:
				solid = new CAlphaBlendColor( FTInfo.params->color, 1, true, true,true);
				shadow = new CAlphaBlendColor( /*FTInfo.params->color*/ShadowColor, FTInfo.params->alpha, true, bDarkColor, true);
				break;
			case FT_PIXEL_MODE_LCD_V:
				solid = new CAlphaBlendColor( FTInfo.params->color, 1, true, true,false);
				shadow = new CAlphaBlendColor( /*FTInfo.params->color*/ShadowColor, FTInfo.params->alpha, true, bDarkColor, false);
				break;
			case FT_PIXEL_MODE_GRAY:
				solid = new CAlphaBlendColor( FTInfo.params->color, 1, false,true, true);
				shadow = new CAlphaBlendColor( /*FTInfo.params->color*/ShadowColor, FTInfo.params->alpha, false, bDarkColor, true);
				break;
			default:
				solid = new CAlphaBlendColor( FTInfo.params->color, 1, pSettings->LcdFilter(),true);
				shadow = new CAlphaBlendColor( /*FTInfo.params->color*/ShadowColor, FTInfo.params->alpha, pSettings->LcdFilter(), bDarkColor);
				break;
	}

//===============�������==========================

	FreeTypeGlyphInfo FTGInfo = {&FTInfo, 0, 0, 0, solid, shadow};
	for (int i=0; i<cbString; ++i, ++lpString)
	{
		WCHAR wch = *lpString;
		if (Glyphs[i])
		{
			FTGInfo.wch = wch;
			FTGInfo.FTGlyph = (FT_BitmapGlyph)(Glyphs[i]);
			FTGInfo.AAMode = FTInfo.AAModes[i];
			TextOutCallback(FTGInfo);
		}
		else
		{
			int j = i;
			FT_DRAW_STATE st= drState[i];
			while (++j<cbString && !Glyphs[j] && drState[j]==st){};
			if (st==FT_DRAW_EMBEDDED_BITMAP)
				ORIG_ExtTextOutW(hdc, FTInfo.x, FTInfo.yTop, FTInfo.GetETO() & ETO_IGNORELANGUAGE, NULL, lpString, j-i, FTInfo.lpDx ? FTInfo.lpDx + i:NULL);
			else
				ORIG_ExtTextOutW(hdc, FTInfo.x, FTInfo.yTop, FTInfo.GetETO(), NULL, lpString, j-i, FTInfo.lpDx ? FTInfo.lpDx + i:NULL);
			lpString += --j - i;
			i = j;
		}
		FTInfo.x=FTInfo.Dx[i];
	}

	if (shadow)
		delete shadow;
	if (solid)
		delete solid;

	int x = FTInfo.x;
	int y = FTInfo.yBase;

	// ������(�����)����

	if(lf.lfUnderline || lf.lfStrikeOut) {
		OUTLINETEXTMETRIC &otm = *FTInfo.params->otm;
		if(lf.lfUnderline){
			int yPos = 0; //�����̈ʒu
			int height = 0;
			int thickness = 0; // �K���ȑ���
			switch (pSettings->FontLoader()) {
			case SETTING_FONTLOADER_FREETYPE:
				yPos = y - otm.otmsUnderscorePosition;
				height = otm.otmTextMetrics.tmHeight; //FT_PosToInt(freetype_face->size->metrics.height);
				thickness =
					MulDiv(freetype_face->underline_thickness,
						FTInfo.font_type.height/*freetype_face->size->metrics.y_ppem*/,
						freetype_face->units_per_EM);
				break;
			case SETTING_FONTLOADER_WIN32:
				yPos = y - otm.otmsUnderscorePosition;
				height = otm.otmTextMetrics.tmHeight;
				thickness = otm.otmsUnderscoreSize;
				break;
			}
			if (yPos >= height) {
				yPos = height - 1;
			}
			cache.DrawHorizontalLine(0, yPos, x, FTInfo.Color(), thickness);
		}

		if(lf.lfStrikeOut){
			int yPos = y - otm.otmsStrikeoutPosition; 
			int thickness = 0; 
			switch (pSettings->FontLoader()) {
			case SETTING_FONTLOADER_FREETYPE:
				thickness =
					MulDiv(freetype_face->underline_thickness,
						FTInfo.font_type.height,// freetype_face->size->metrics.y_ppem,
						freetype_face->units_per_EM);
				break;
			case SETTING_FONTLOADER_WIN32:
				thickness = otm.otmsStrikeoutSize;
				break;
			}
			cache.DrawHorizontalLine(0, yPos, x, FTInfo.Color(), thickness);
		}
	}
	return TRUE;
}

BOOL FreeTypeGetGlyph(	//�������ͼ�κ���Ҫ�Ŀ���
					 FreeTypeDrawInfo& FTInfo,
					 LPCWSTR lpString,  
					 int cbString,     
					 int& width,
					 FT_Glyph* Glyphs,
					 FT_DRAW_STATE* drState
					 )
{
	COwnedCriticalSectionLock __lock(1);
	{
		//CCriticalSectionLock __lock;
		if (!FreeTypePrepare(FTInfo))
			return false;
	}
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	BOOL nRet = false;
	switch (pSettings->FontLoader()) {
			case SETTING_FONTLOADER_FREETYPE:
				nRet = ForEachGetGlyphFT(FTInfo,lpString,cbString,Glyphs,drState);
				break;
			case SETTING_FONTLOADER_WIN32:
				nRet = ForEachGetGlyphGGO(FTInfo,lpString,cbString,Glyphs,drState);
				break;
	}
	width = FTInfo.px;	//����˿���
	return nRet;
}



void VertFinalizer(void *object){
	FT_Face face = (FT_Face)object;
	ft2vert_final(face, (struct ft2vert_st *)face->generic.data);
}
//
// �O���t��IVS�Ŏw�肳�ꂽ���`���T�|�[�g���邩�ǂ������ׁA
// �T�|�[�g���Ă���ꍇ�̓O���t��u������B
// �T�|�[�g���Ă��Ȃ���Ή������Ȃ��B
//
/*
void FreeTypeSubstGlyph(const HDC hdc, 
						const WORD vsindex,
						const int baseChar,
						int cChars, 
						SCRIPT_ANALYSIS* psa, 
						WORD* pwOutGlyphs, 
						WORD* pwLogClust, 
						SCRIPT_VISATTR* psva, 
						int* pcGlyphs 
						)
{
	CThreadLocalInfo* pTLInfo = g_TLInfo.GetPtr();
	CBitmapCache& cache = pTLInfo->BitmapCache();
	CCriticalSectionLock __lock;

	LOGFONT lf;
	if (!GetLogFontFromDC(hdc, lf))
		return;

	FREETYPE_PARAMS params(0, hdc);
	FreeTypeDrawInfo FTInfo(params, hdc, &lf, &cache);
	if(!FreeTypePrepare(FTInfo))
		return;

	FT_UInt glyph_index = ft2_subst_uvs(FTInfo.freetype_face, pwOutGlyphs[*pcGlyphs - 1], vsindex, baseChar);
	TRACE(_T("FreeTypeSubstGlyph: %04X->%04X\n"), pwOutGlyphs[*pcGlyphs - 1], glyph_index);
	if (glyph_index) {
		pwOutGlyphs[*pcGlyphs - 1] = glyph_index; // �u�������s
		// ASCII�󔒂̃O���t���擾
		glyph_index = FTC_CMapCache_Lookup(
			cmap_cache,
			FTInfo.font_type.face_id,
			FTInfo.cmap_index,
			' ');
		// �[�����O���t�ɂ���
		pwOutGlyphs[*pcGlyphs] = glyph_index;
		psva[*pcGlyphs].uJustification = SCRIPT_JUSTIFY_NONE;
		psva[*pcGlyphs].fClusterStart = 0;
		psva[*pcGlyphs].fDiacritic = 0;
		psva[*pcGlyphs].fZeroWidth = 1;
		psva[*pcGlyphs].fReserved = 0;
		psva[*pcGlyphs].fShapeReserved = 0;
	} else {
		// �t�H���g�͎w�肳�ꂽ���`�������Ȃ��BIVS�̃O���t���擾
		glyph_index = FTC_CMapCache_Lookup(
			cmap_cache,
			FTInfo.font_type.face_id,
			FTInfo.cmap_index,
			vsindex + 0xE0100);
		// IVS���T�|�[�g���Ă��Ȃ��t�H���g��IVS�̃O���t�������Ă���\�����قƂ�ǂȂ��B
		// missing glyph��Ԃ��ƃt�H�[���o�b�N����Ă��܂����ߊm���Ɏ����Ă������ȃO���t���E��
		if (!glyph_index)
			glyph_index = FTC_CMapCache_Lookup(
			cmap_cache,
			FTInfo.font_type.face_id,
			FTInfo.cmap_index,
			0x30FB);
		pwOutGlyphs[*pcGlyphs] = glyph_index;
		psva[*pcGlyphs] = psva[*pcGlyphs - 1];
		psva[*pcGlyphs].fClusterStart = 0;
	}
	pwLogClust[cChars - 2] = *pcGlyphs;
	pwLogClust[cChars - 1] = *pcGlyphs;
	++*pcGlyphs;
}*/


FT_Error face_requester(
		FTC_FaceID face_id,
		FT_Library /*library*/,
		FT_Pointer /*request_data*/,
		FT_Face* aface)
{
	FT_Error ret;
	FT_Face face;

	FreeTypeFontInfo* pfi = g_pFTEngine->FindFont((int)face_id);
	Assert(pfi);
	if (!pfi) {
		return FT_Err_Invalid_Argument;
	}
	LPCTSTR fontname = pfi->GetName();

	// ���̂��w�肵�ăt�H���g���擾
	FreeTypeSysFontData* pData = FreeTypeSysFontData::CreateInstance(fontname, pfi->GetFontWeight(), pfi->IsItalic());
	if(pData == NULL){
		return FT_Err_Cannot_Open_Resource;
	}

	face = pData->GetFace();
	Assert(face != NULL);

	// Charmap��ݒ肵�Ă���
	ret = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_MS_SYMBOL);
/*
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_SJIS);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_GB2312);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_BIG5);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_WANSUNG);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_JOHAB);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_ADOBE_STANDARD);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_ADOBE_EXPERT);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_ADOBE_CUSTOM);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_ADOBE_LATIN_1);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_OLD_LATIN_2);
	if(ret != FT_Err_Ok)
		ret = FT_Select_Charmap(face, FT_ENCODING_APPLE_ROMAN); */

	if(ret != FT_Err_Ok)
	{
		FT_Done_Face(face);
		return ret;
	}
	struct ft2vert_st *vert = ft2vert_init(face);
	face->generic.data = vert;
	face->generic.finalizer = VertFinalizer;

	*aface = face;
	return 0;
}


/*
DWORD FreeTypeGetVersion()
{
	int major = 0, minor = 0, patch = 0;
	FT_Library_Version(freetype_library, &major, &minor, &patch);
	//�ʓ|�Ȃ̂�RGB�}�N���g�p
	return RGB(major, minor, patch);
}*/


//�V�����A���S���Y��
FT_Error New_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos str_h, FT_Pos str_v, FT_Int font_size )
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	int orientation = 0;
	switch (pSettings->BolderMode()) {
	case 1:
		return FT_Outline_EmboldenXY( outline, str_h, 0);

	case 2:
		return FT_Outline_Embolden( outline, str_h );

	default:
		{
			if ( !outline ) return FT_Err_Invalid_Argument;
			//orientation = FT_Outline_Get_Orientation( outline );
			//if ( orientation == FT_ORIENTATION_NONE )
			//	if ( outline->n_contours ) return FT_Err_Invalid_Argument;
/*
			if (font_size>FT_BOLD_LOW || str_h<16)
				Vert_FT_Outline_Embolden( outline, str_v );
			Old_FT_Outline_Embolden( outline, str_h );*/
			if (font_size<FT_BOLD_LOW && str_h>32)
			{
				FT_Outline_EmboldenXY(outline, str_h, Min(long(32), str_v));
			}
			else
				FT_Outline_Embolden(outline, str_h);
			return FT_Err_Ok;
		}
	}
}

//�������������点��FT_Outline_Embolden
FT_Error Old_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos strength )
{
	FT_Vector*	points;
	FT_Vector	v_prev, v_first, v_next, v_cur;
	FT_Angle	rotate, angle_in, angle_out;
	FT_Int		c, n, first;
	FT_Int		orientation;

	if ( !outline )
		return FT_Err_Invalid_Argument;

	strength /= 2;
	if ( strength == 0 )
		return FT_Err_Ok;

	orientation = FT_Outline_Get_Orientation( outline );
	if ( orientation == FT_ORIENTATION_NONE )
	{
		if ( outline->n_contours )
			return FT_Err_Invalid_Argument;
		else
			return FT_Err_Ok;
	}

	if ( orientation == FT_ORIENTATION_TRUETYPE )
		rotate = -FT_ANGLE_PI2;
	else
		rotate = FT_ANGLE_PI2;

	points = outline->points;

	first = 0;
	for ( c = 0; c < outline->n_contours; c++ )
	{
		int  last = outline->contours[c];

		v_first = points[first];
		v_prev  = points[last];
		v_cur   = v_first;

		for ( n = first; n <= last; n++ )
		{
			FT_Vector	in, out;
			FT_Angle	angle_diff;
			FT_Pos		d;
			FT_Fixed	scale;

			if ( n < last )
				v_next = points[n + 1];
			else
				v_next = v_first;

			/* compute the in and out vectors */
			in.x = v_cur.x - v_prev.x;
			in.y = v_cur.y - v_prev.y;

			out.x = v_next.x - v_cur.x;
			out.y = v_next.y - v_cur.y;

			angle_in   = FT_Atan2( in.x, in.y );
			angle_out  = FT_Atan2( out.x, out.y );
			angle_diff = FT_Angle_Diff( angle_in, angle_out );
			scale      = FT_Cos( angle_diff / 2 );

			if ( scale < 0x4000L && scale > -0x4000L )
				in.x = in.y = 0;
			else
			{
				d = FT_DivFix( strength, scale );

				FT_Vector_From_Polar( &in, d, angle_in + angle_diff / 2 - rotate );
			}

			outline->points[n].x = v_cur.x + strength + in.x;
			//��������R�����g�A�E�g��������
			//outline->points[n].y = v_cur.y + strength + in.y;

			v_prev = v_cur;
			v_cur  = v_next;
		}

		first = last + 1;
	}

	return FT_Err_Ok;
}

//�������͏c����
FT_Error Vert_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos strength )
{
	FT_Vector*	points;
	FT_Vector	v_prev, v_first, v_next, v_cur;
	FT_Angle	rotate, angle_in, angle_out;
	FT_Int		c, n, first;
	FT_Int		orientation;

	if ( !outline )
		return FT_Err_Invalid_Argument;

	strength /= 2;
	if ( strength == 0 )
		return FT_Err_Ok;

	orientation = FT_Outline_Get_Orientation( outline );
	if ( orientation == FT_ORIENTATION_NONE )
	{
		if ( outline->n_contours )
			return FT_Err_Invalid_Argument;
		else
			return FT_Err_Ok;
	}

	if ( orientation == FT_ORIENTATION_TRUETYPE )
		rotate = -FT_ANGLE_PI2;
	else
		rotate = FT_ANGLE_PI2;

	points = outline->points;

	first = 0;
	for ( c = 0; c < outline->n_contours; c++ )
	{
		int  last = outline->contours[c];

		v_first = points[first];
		v_prev  = points[last];
		v_cur   = v_first;

		for ( n = first; n <= last; n++ )
		{
			FT_Vector	in, out;
			FT_Angle	angle_diff;
			FT_Pos		d;
			FT_Fixed	scale;

			if ( n < last )
				v_next = points[n + 1];
			else
				v_next = v_first;

			/* compute the in and out vectors */
			in.x = v_cur.x - v_prev.x;
			in.y = v_cur.y - v_prev.y;

			out.x = v_next.x - v_cur.x;
			out.y = v_next.y - v_cur.y;

			angle_in   = FT_Atan2( in.x, in.y );
			angle_out  = FT_Atan2( out.x, out.y );
			angle_diff = FT_Angle_Diff( angle_in, angle_out );
			scale      = FT_Cos( angle_diff / 2 );

			if ( scale < 0x4000L && scale > -0x4000L )
				in.x = in.y = 0;
			else
			{
				d = FT_DivFix( strength, scale );

				FT_Vector_From_Polar( &in, d, angle_in + angle_diff / 2 - rotate );
			}

			//outline->points[n].x = v_cur.x + strength + in.x;
			//��������R�����g�A�E�g��������
			outline->points[n].y = v_cur.y + strength + in.y;

			v_prev = v_cur;
			v_cur  = v_next;
		}

		first = last + 1;
	}

	return FT_Err_Ok;
}

//�_�~�[
FT_EXPORT( FT_Error )
FT_Library_SetLcdFilter_Dummy( FT_Library    /*library*/,
                               FT_LcdFilter  /*filter*/ )
{
	return 0;
}

BOOL FontLInit(void){
	CCriticalSectionLock __lock;

	if(FT_Init_FreeType(&freetype_library)){
		return FALSE;
	}
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	if(FTC_Manager_New(freetype_library,
		pSettings->CacheMaxFaces(),
		pSettings->CacheMaxSizes(),
		pSettings->CacheMaxBytes(),
		face_requester, NULL,
		&cache_man))
	{
		FontLFree();
		return FALSE;
	}
	if(FTC_CMapCache_New(cache_man, &cmap_cache)){
		FontLFree();
		return FALSE;
	}
	if(FTC_ImageCache_New(cache_man, &image_cache)){
		FontLFree();
		return FALSE;
	}

	const int nLcdFilter = pSettings->LcdFilter();
	if ((int)FT_LCD_FILTER_NONE < nLcdFilter && nLcdFilter < (int)FT_LCD_FILTER_MAX) {
		FT_Library_SetLcdFilter(freetype_library, (FT_LcdFilter)nLcdFilter);
		if (pSettings->UseCustomLcdFilter())
		{
			unsigned char buff[5];
			memcpy(buff, pSettings->LcdFilterWeights(), sizeof(buff));
			FT_Library_SetLcdFilterWeights(freetype_library, buff);
		}
		else
			switch (nLcdFilter)
			{
			case FT_LCD_FILTER_NONE:
			case FT_LCD_FILTER_DEFAULT:
			case FT_LCD_FILTER_LEGACY:
				{
					FT_Library_SetLcdFilterWeights(freetype_library,
						(unsigned char*)"\x00\x55\x56\x55\x00" );
					break;
				}
			case FT_LCD_FILTER_LIGHT:
			default:
				FT_Library_SetLcdFilterWeights(freetype_library,
					(unsigned char*)"\x10\x40\x70\x40\x10" );
			}
	}
	s_AlphaBlendTable.init();
	s_AlphaBlendTable.initRGB();

	return TRUE;
}

void FontLFree(void){
	CCriticalSectionLock __lock;

	if(cache_man != NULL)
		FTC_Manager_Done(cache_man);
	if(freetype_library != NULL)
		FT_Done_FreeType(freetype_library);
	cache_man = NULL;
	freetype_library = NULL;
}

//Snowie
void RefreshAlphaTable()
{
	s_AlphaBlendTable.init();
}

void UpdateLcdFilter()
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	const int nLcdFilter = pSettings->LcdFilter();
	if ((int)FT_LCD_FILTER_NONE < nLcdFilter && nLcdFilter < (int)FT_LCD_FILTER_MAX) {
		FT_Library_SetLcdFilter(freetype_library, (FT_LcdFilter)nLcdFilter);
		if (pSettings->UseCustomLcdFilter())
		{
			unsigned char buff[5];
			memcpy(buff, pSettings->LcdFilterWeights(), sizeof(buff));
			FT_Library_SetLcdFilterWeights(freetype_library, buff);
		}
/*
		else
			switch (nLcdFilter)
		{
			case FT_LCD_FILTER_NONE:
			case FT_LCD_FILTER_DEFAULT:
			case FT_LCD_FILTER_LEGACY:
				{
					FT_Library_SetLcdFilterWeights(freetype_library,
						(unsigned char*)"\x10\x40\x70\x40\x10" );
					break;
				}
			case FT_LCD_FILTER_LIGHT:
			default:
				FT_Library_SetLcdFilterWeights(freetype_library,
					(unsigned char*)"\x00\x55\x56\x55\x00" );
		}*/

	}
}
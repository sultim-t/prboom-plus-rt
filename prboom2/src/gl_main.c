/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_main.c,v 1.29 2000/09/30 17:31:13 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 *---------------------------------------------------------------------
 */

#include "gl_intern.h"
#include "gl_struct.h"

extern int tran_filter_pct;

boolean use_fog=false;
boolean use_maindisplay_list=false;
boolean use_display_lists=false;

GLuint gld_DisplayList=0;
int fog_density=200;
static float extra_red=0.0f;
static float extra_green=0.0f;
static float extra_blue=0.0f;
static float extra_alpha=0.0f;

#define MAP_COEFF 128
#define MAP_SCALE	(MAP_COEFF<<FRACBITS) // 6553600 -- nicolas

/*
 * lookuptable for lightvalues
 * calculated as follow:
 * floatlight=(1.0-exp((light^3)*gamma)) / (1.0-exp(1.0*gamma));
 * gamma=-0,2;-2,0;-4,0;-6,0;-8,0
 * light=0,0 .. 1,0
 */
static const float lighttable[5][256] =
{
  {
    0.00000f,0.00000f,0.00000f,0.00000f,0.00000f,0.00001f,0.00001f,0.00002f,0.00003f,0.00004f,
    0.00006f,0.00008f,0.00010f,0.00013f,0.00017f,0.00020f,0.00025f,0.00030f,0.00035f,0.00041f,
    0.00048f,0.00056f,0.00064f,0.00073f,0.00083f,0.00094f,0.00106f,0.00119f,0.00132f,0.00147f,
    0.00163f,0.00180f,0.00198f,0.00217f,0.00237f,0.00259f,0.00281f,0.00305f,0.00331f,0.00358f,
    0.00386f,0.00416f,0.00447f,0.00479f,0.00514f,0.00550f,0.00587f,0.00626f,0.00667f,0.00710f,
    0.00754f,0.00800f,0.00848f,0.00898f,0.00950f,0.01003f,0.01059f,0.01117f,0.01177f,0.01239f,
    0.01303f,0.01369f,0.01437f,0.01508f,0.01581f,0.01656f,0.01734f,0.01814f,0.01896f,0.01981f,
    0.02069f,0.02159f,0.02251f,0.02346f,0.02444f,0.02544f,0.02647f,0.02753f,0.02862f,0.02973f,
    0.03088f,0.03205f,0.03325f,0.03448f,0.03575f,0.03704f,0.03836f,0.03971f,0.04110f,0.04252f,
    0.04396f,0.04545f,0.04696f,0.04851f,0.05009f,0.05171f,0.05336f,0.05504f,0.05676f,0.05852f,
    0.06031f,0.06214f,0.06400f,0.06590f,0.06784f,0.06981f,0.07183f,0.07388f,0.07597f,0.07810f,
    0.08027f,0.08248f,0.08473f,0.08702f,0.08935f,0.09172f,0.09414f,0.09659f,0.09909f,0.10163f,
    0.10421f,0.10684f,0.10951f,0.11223f,0.11499f,0.11779f,0.12064f,0.12354f,0.12648f,0.12946f,
    0.13250f,0.13558f,0.13871f,0.14188f,0.14511f,0.14838f,0.15170f,0.15507f,0.15850f,0.16197f,
    0.16549f,0.16906f,0.17268f,0.17635f,0.18008f,0.18386f,0.18769f,0.19157f,0.19551f,0.19950f,
    0.20354f,0.20764f,0.21179f,0.21600f,0.22026f,0.22458f,0.22896f,0.23339f,0.23788f,0.24242f,
    0.24702f,0.25168f,0.25640f,0.26118f,0.26602f,0.27091f,0.27587f,0.28089f,0.28596f,0.29110f,
    0.29630f,0.30156f,0.30688f,0.31226f,0.31771f,0.32322f,0.32879f,0.33443f,0.34013f,0.34589f,
    0.35172f,0.35761f,0.36357f,0.36960f,0.37569f,0.38185f,0.38808f,0.39437f,0.40073f,0.40716f,
    0.41366f,0.42022f,0.42686f,0.43356f,0.44034f,0.44718f,0.45410f,0.46108f,0.46814f,0.47527f,
    0.48247f,0.48974f,0.49709f,0.50451f,0.51200f,0.51957f,0.52721f,0.53492f,0.54271f,0.55058f,
    0.55852f,0.56654f,0.57463f,0.58280f,0.59105f,0.59937f,0.60777f,0.61625f,0.62481f,0.63345f,
    0.64217f,0.65096f,0.65984f,0.66880f,0.67783f,0.68695f,0.69615f,0.70544f,0.71480f,0.72425f,
    0.73378f,0.74339f,0.75308f,0.76286f,0.77273f,0.78268f,0.79271f,0.80283f,0.81304f,0.82333f,
    0.83371f,0.84417f,0.85472f,0.86536f,0.87609f,0.88691f,0.89781f,0.90880f,0.91989f,0.93106f,
    0.94232f,0.95368f,0.96512f,0.97665f,0.98828f,1.00000
  },
  {
    0.00000f,0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00003f,0.00005f,0.00007f,0.00010f,
    0.00014f,0.00019f,0.00024f,0.00031f,0.00038f,0.00047f,0.00057f,0.00069f,0.00081f,0.00096f,
    0.00112f,0.00129f,0.00148f,0.00170f,0.00193f,0.00218f,0.00245f,0.00274f,0.00306f,0.00340f,
    0.00376f,0.00415f,0.00456f,0.00500f,0.00547f,0.00597f,0.00649f,0.00704f,0.00763f,0.00825f,
    0.00889f,0.00957f,0.01029f,0.01104f,0.01182f,0.01264f,0.01350f,0.01439f,0.01532f,0.01630f,
    0.01731f,0.01836f,0.01945f,0.02058f,0.02176f,0.02298f,0.02424f,0.02555f,0.02690f,0.02830f,
    0.02974f,0.03123f,0.03277f,0.03436f,0.03600f,0.03768f,0.03942f,0.04120f,0.04304f,0.04493f,
    0.04687f,0.04886f,0.05091f,0.05301f,0.05517f,0.05738f,0.05964f,0.06196f,0.06434f,0.06677f,
    0.06926f,0.07181f,0.07441f,0.07707f,0.07979f,0.08257f,0.08541f,0.08831f,0.09126f,0.09428f,
    0.09735f,0.10048f,0.10368f,0.10693f,0.11025f,0.11362f,0.11706f,0.12056f,0.12411f,0.12773f,
    0.13141f,0.13515f,0.13895f,0.14281f,0.14673f,0.15072f,0.15476f,0.15886f,0.16303f,0.16725f,
    0.17153f,0.17587f,0.18028f,0.18474f,0.18926f,0.19383f,0.19847f,0.20316f,0.20791f,0.21272f,
    0.21759f,0.22251f,0.22748f,0.23251f,0.23760f,0.24274f,0.24793f,0.25318f,0.25848f,0.26383f,
    0.26923f,0.27468f,0.28018f,0.28573f,0.29133f,0.29697f,0.30266f,0.30840f,0.31418f,0.32001f,
    0.32588f,0.33179f,0.33774f,0.34374f,0.34977f,0.35585f,0.36196f,0.36810f,0.37428f,0.38050f,
    0.38675f,0.39304f,0.39935f,0.40570f,0.41207f,0.41847f,0.42490f,0.43136f,0.43784f,0.44434f,
    0.45087f,0.45741f,0.46398f,0.47057f,0.47717f,0.48379f,0.49042f,0.49707f,0.50373f,0.51041f,
    0.51709f,0.52378f,0.53048f,0.53718f,0.54389f,0.55061f,0.55732f,0.56404f,0.57075f,0.57747f,
    0.58418f,0.59089f,0.59759f,0.60429f,0.61097f,0.61765f,0.62432f,0.63098f,0.63762f,0.64425f,
    0.65086f,0.65746f,0.66404f,0.67060f,0.67714f,0.68365f,0.69015f,0.69662f,0.70307f,0.70948f,
    0.71588f,0.72224f,0.72857f,0.73488f,0.74115f,0.74739f,0.75359f,0.75976f,0.76589f,0.77199f,
    0.77805f,0.78407f,0.79005f,0.79599f,0.80189f,0.80774f,0.81355f,0.81932f,0.82504f,0.83072f,
    0.83635f,0.84194f,0.84747f,0.85296f,0.85840f,0.86378f,0.86912f,0.87441f,0.87964f,0.88482f,
    0.88995f,0.89503f,0.90005f,0.90502f,0.90993f,0.91479f,0.91959f,0.92434f,0.92903f,0.93366f,
    0.93824f,0.94276f,0.94723f,0.95163f,0.95598f,0.96027f,0.96451f,0.96868f,0.97280f,0.97686f,
    0.98086f,0.98481f,0.98869f,0.99252f,0.99629f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00003f,0.00005f,0.00008f,0.00013f,0.00018f,
    0.00025f,0.00033f,0.00042f,0.00054f,0.00067f,0.00083f,0.00101f,0.00121f,0.00143f,0.00168f,
    0.00196f,0.00227f,0.00261f,0.00299f,0.00339f,0.00383f,0.00431f,0.00483f,0.00538f,0.00598f,
    0.00661f,0.00729f,0.00802f,0.00879f,0.00961f,0.01048f,0.01140f,0.01237f,0.01340f,0.01447f,
    0.01561f,0.01680f,0.01804f,0.01935f,0.02072f,0.02215f,0.02364f,0.02520f,0.02682f,0.02850f,
    0.03026f,0.03208f,0.03397f,0.03594f,0.03797f,0.04007f,0.04225f,0.04451f,0.04684f,0.04924f,
    0.05172f,0.05428f,0.05691f,0.05963f,0.06242f,0.06530f,0.06825f,0.07129f,0.07441f,0.07761f,
    0.08089f,0.08426f,0.08771f,0.09125f,0.09487f,0.09857f,0.10236f,0.10623f,0.11019f,0.11423f,
    0.11836f,0.12257f,0.12687f,0.13125f,0.13571f,0.14027f,0.14490f,0.14962f,0.15442f,0.15931f,
    0.16427f,0.16932f,0.17445f,0.17966f,0.18496f,0.19033f,0.19578f,0.20130f,0.20691f,0.21259f,
    0.21834f,0.22417f,0.23007f,0.23605f,0.24209f,0.24820f,0.25438f,0.26063f,0.26694f,0.27332f,
    0.27976f,0.28626f,0.29282f,0.29944f,0.30611f,0.31284f,0.31962f,0.32646f,0.33334f,0.34027f,
    0.34724f,0.35426f,0.36132f,0.36842f,0.37556f,0.38273f,0.38994f,0.39718f,0.40445f,0.41174f,
    0.41907f,0.42641f,0.43378f,0.44116f,0.44856f,0.45598f,0.46340f,0.47084f,0.47828f,0.48573f,
    0.49319f,0.50064f,0.50809f,0.51554f,0.52298f,0.53042f,0.53784f,0.54525f,0.55265f,0.56002f,
    0.56738f,0.57472f,0.58203f,0.58932f,0.59658f,0.60381f,0.61101f,0.61817f,0.62529f,0.63238f,
    0.63943f,0.64643f,0.65339f,0.66031f,0.66717f,0.67399f,0.68075f,0.68746f,0.69412f,0.70072f,
    0.70726f,0.71375f,0.72017f,0.72653f,0.73282f,0.73905f,0.74522f,0.75131f,0.75734f,0.76330f,
    0.76918f,0.77500f,0.78074f,0.78640f,0.79199f,0.79751f,0.80295f,0.80831f,0.81359f,0.81880f,
    0.82393f,0.82898f,0.83394f,0.83883f,0.84364f,0.84836f,0.85301f,0.85758f,0.86206f,0.86646f,
    0.87078f,0.87502f,0.87918f,0.88326f,0.88726f,0.89118f,0.89501f,0.89877f,0.90245f,0.90605f,
    0.90957f,0.91301f,0.91638f,0.91966f,0.92288f,0.92601f,0.92908f,0.93206f,0.93498f,0.93782f,
    0.94059f,0.94329f,0.94592f,0.94848f,0.95097f,0.95339f,0.95575f,0.95804f,0.96027f,0.96244f,
    0.96454f,0.96658f,0.96856f,0.97049f,0.97235f,0.97416f,0.97591f,0.97760f,0.97924f,0.98083f,
    0.98237f,0.98386f,0.98530f,0.98669f,0.98803f,0.98933f,0.99058f,0.99179f,0.99295f,0.99408f,
    0.99516f,0.99620f,0.99721f,0.99817f,0.99910f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00002f,0.00005f,0.00008f,0.00012f,0.00019f,0.00026f,
    0.00036f,0.00048f,0.00063f,0.00080f,0.00099f,0.00122f,0.00148f,0.00178f,0.00211f,0.00249f,
    0.00290f,0.00335f,0.00386f,0.00440f,0.00500f,0.00565f,0.00636f,0.00711f,0.00793f,0.00881f,
    0.00975f,0.01075f,0.01182f,0.01295f,0.01416f,0.01543f,0.01678f,0.01821f,0.01971f,0.02129f,
    0.02295f,0.02469f,0.02652f,0.02843f,0.03043f,0.03252f,0.03469f,0.03696f,0.03933f,0.04178f,
    0.04433f,0.04698f,0.04973f,0.05258f,0.05552f,0.05857f,0.06172f,0.06498f,0.06834f,0.07180f,
    0.07537f,0.07905f,0.08283f,0.08672f,0.09072f,0.09483f,0.09905f,0.10337f,0.10781f,0.11236f,
    0.11701f,0.12178f,0.12665f,0.13163f,0.13673f,0.14193f,0.14724f,0.15265f,0.15817f,0.16380f,
    0.16954f,0.17538f,0.18132f,0.18737f,0.19351f,0.19976f,0.20610f,0.21255f,0.21908f,0.22572f,
    0.23244f,0.23926f,0.24616f,0.25316f,0.26023f,0.26739f,0.27464f,0.28196f,0.28935f,0.29683f,
    0.30437f,0.31198f,0.31966f,0.32740f,0.33521f,0.34307f,0.35099f,0.35896f,0.36699f,0.37506f,
    0.38317f,0.39133f,0.39952f,0.40775f,0.41601f,0.42429f,0.43261f,0.44094f,0.44929f,0.45766f,
    0.46604f,0.47443f,0.48283f,0.49122f,0.49962f,0.50801f,0.51639f,0.52476f,0.53312f,0.54146f,
    0.54978f,0.55807f,0.56633f,0.57457f,0.58277f,0.59093f,0.59905f,0.60713f,0.61516f,0.62314f,
    0.63107f,0.63895f,0.64676f,0.65452f,0.66221f,0.66984f,0.67739f,0.68488f,0.69229f,0.69963f,
    0.70689f,0.71407f,0.72117f,0.72818f,0.73511f,0.74195f,0.74870f,0.75536f,0.76192f,0.76839f,
    0.77477f,0.78105f,0.78723f,0.79331f,0.79930f,0.80518f,0.81096f,0.81664f,0.82221f,0.82768f,
    0.83305f,0.83832f,0.84347f,0.84853f,0.85348f,0.85832f,0.86306f,0.86770f,0.87223f,0.87666f,
    0.88098f,0.88521f,0.88933f,0.89334f,0.89726f,0.90108f,0.90480f,0.90842f,0.91194f,0.91537f,
    0.91870f,0.92193f,0.92508f,0.92813f,0.93109f,0.93396f,0.93675f,0.93945f,0.94206f,0.94459f,
    0.94704f,0.94941f,0.95169f,0.95391f,0.95604f,0.95810f,0.96009f,0.96201f,0.96386f,0.96564f,
    0.96735f,0.96900f,0.97059f,0.97212f,0.97358f,0.97499f,0.97634f,0.97764f,0.97888f,0.98007f,
    0.98122f,0.98231f,0.98336f,0.98436f,0.98531f,0.98623f,0.98710f,0.98793f,0.98873f,0.98949f,
    0.99021f,0.99090f,0.99155f,0.99218f,0.99277f,0.99333f,0.99387f,0.99437f,0.99486f,0.99531f,
    0.99575f,0.99616f,0.99654f,0.99691f,0.99726f,0.99759f,0.99790f,0.99819f,0.99847f,0.99873f,
    0.99897f,0.99920f,0.99942f,0.99963f,0.99982f,1.00000f
  },
  {
    0.00000f,0.00000f,0.00000f,0.00001f,0.00003f,0.00006f,0.00010f,0.00017f,0.00025f,0.00035f,
    0.00048f,0.00064f,0.00083f,0.00106f,0.00132f,0.00163f,0.00197f,0.00237f,0.00281f,0.00330f,
    0.00385f,0.00446f,0.00513f,0.00585f,0.00665f,0.00751f,0.00845f,0.00945f,0.01054f,0.01170f,
    0.01295f,0.01428f,0.01569f,0.01719f,0.01879f,0.02048f,0.02227f,0.02415f,0.02614f,0.02822f,
    0.03042f,0.03272f,0.03513f,0.03765f,0.04028f,0.04303f,0.04589f,0.04887f,0.05198f,0.05520f,
    0.05855f,0.06202f,0.06561f,0.06933f,0.07318f,0.07716f,0.08127f,0.08550f,0.08987f,0.09437f,
    0.09900f,0.10376f,0.10866f,0.11369f,0.11884f,0.12414f,0.12956f,0.13512f,0.14080f,0.14662f,
    0.15257f,0.15865f,0.16485f,0.17118f,0.17764f,0.18423f,0.19093f,0.19776f,0.20471f,0.21177f,
    0.21895f,0.22625f,0.23365f,0.24117f,0.24879f,0.25652f,0.26435f,0.27228f,0.28030f,0.28842f,
    0.29662f,0.30492f,0.31329f,0.32175f,0.33028f,0.33889f,0.34756f,0.35630f,0.36510f,0.37396f,
    0.38287f,0.39183f,0.40084f,0.40989f,0.41897f,0.42809f,0.43723f,0.44640f,0.45559f,0.46479f,
    0.47401f,0.48323f,0.49245f,0.50167f,0.51088f,0.52008f,0.52927f,0.53843f,0.54757f,0.55668f,
    0.56575f,0.57479f,0.58379f,0.59274f,0.60164f,0.61048f,0.61927f,0.62799f,0.63665f,0.64524f,
    0.65376f,0.66220f,0.67056f,0.67883f,0.68702f,0.69511f,0.70312f,0.71103f,0.71884f,0.72655f,
    0.73415f,0.74165f,0.74904f,0.75632f,0.76348f,0.77053f,0.77747f,0.78428f,0.79098f,0.79756f,
    0.80401f,0.81035f,0.81655f,0.82264f,0.82859f,0.83443f,0.84013f,0.84571f,0.85117f,0.85649f,
    0.86169f,0.86677f,0.87172f,0.87654f,0.88124f,0.88581f,0.89026f,0.89459f,0.89880f,0.90289f,
    0.90686f,0.91071f,0.91445f,0.91807f,0.92157f,0.92497f,0.92826f,0.93143f,0.93450f,0.93747f,
    0.94034f,0.94310f,0.94577f,0.94833f,0.95081f,0.95319f,0.95548f,0.95768f,0.95980f,0.96183f,
    0.96378f,0.96565f,0.96744f,0.96916f,0.97081f,0.97238f,0.97388f,0.97532f,0.97669f,0.97801f,
    0.97926f,0.98045f,0.98158f,0.98266f,0.98369f,0.98467f,0.98560f,0.98648f,0.98732f,0.98811f,
    0.98886f,0.98958f,0.99025f,0.99089f,0.99149f,0.99206f,0.99260f,0.99311f,0.99359f,0.99404f,
    0.99446f,0.99486f,0.99523f,0.99559f,0.99592f,0.99623f,0.99652f,0.99679f,0.99705f,0.99729f,
    0.99751f,0.99772f,0.99792f,0.99810f,0.99827f,0.99843f,0.99857f,0.99871f,0.99884f,0.99896f,
    0.99907f,0.99917f,0.99926f,0.99935f,0.99943f,0.99951f,0.99958f,0.99964f,0.99970f,0.99975f,
    0.99980f,0.99985f,0.99989f,0.99993f,0.99997f,1.00000f
  }
};

static float gld_CalcLightLevel(int lightlevel)
{
  float light;

  lightlevel=max(min(lightlevel,255),0);
  light=lighttable[usegamma][lightlevel];

  return light;
}

static void gld_StaticLight(float light)
{ 
  player_t *player;
	player = &players[displayplayer];

	if (player->fixedcolormap == 32)
  {
    glColor3f(0.5f, 1.0f, 0.0f);
    return;
  }
	else
  	if (player->fixedcolormap)
    {
      glColor3f(1.0f, 1.0f, 1.0f);
      return;
    }
#ifdef _DEBUG
/*
  if (usingGLNodes)
    glColor4f(light, light, light, 0.7f);
  else
*/
    glColor3f(light, light, light);
#else
  glColor3f(light, light, light);
#endif
}

static void gld_StaticLightAlpha(float light, float alpha)
{ 
  player_t *player;
	player = &players[displayplayer];

	if (player->fixedcolormap == 32)
  {
    glColor4f(0.5f, 1.0f, 0.0f, alpha);
    return;
  }
	else
  	if (player->fixedcolormap)
    {
      glColor4f(1.0f, 1.0f, 1.0f, alpha);
      return;
    }
  glColor4f(light, light, light, alpha);
}

void gld_Init(int width, int height)
{ 
  GLfloat params[4]={0.0f,0.0f,1.0f,0.0f};
	GLfloat BlackFogColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

  lprintf(LO_INFO,"\nGL_VENDOR:\n%s\n",glGetString(GL_VENDOR));
  lprintf(LO_INFO,"GL_RENDERER:\n%s\n",glGetString(GL_RENDERER));
  lprintf(LO_INFO,"GL_VERSION:\n%s\n",glGetString(GL_VERSION));
	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT); 

  glClearColor(0.0f, 0.5f, 0.5f, 1.0f); 
  glClearDepth(1.0f);

  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&gld_max_texturesize);
  //gld_max_texturesize=16;
  lprintf(LO_INFO,"gld_max_texturesize=%i\n",gld_max_texturesize);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // proff_dis
  glShadeModel(GL_FLAT);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LEQUAL);
 	glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GEQUAL,0.5f);
	glDisable(GL_CULL_FACE);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

  glTexGenfv(GL_Q,GL_EYE_PLANE,params);
  glTexGenf(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
  glTexGenf(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
  glTexGenf(GL_Q,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glFogi (GL_FOG_MODE, GL_EXP);
	glFogfv(GL_FOG_COLOR, BlackFogColor);
	glFogf (GL_FOG_DENSITY, (float)fog_density/1000.0f);
	glHint (GL_FOG_HINT, GL_NICEST);
	glFogf (GL_FOG_START, 0.0f);
	glFogf (GL_FOG_END, 1.0f);
}

void gld_InitCommandLine()
{
}

#define SCALE_X(x)		((flags & VPT_STRETCH)?((float)x)*(float)SCREENWIDTH/320.0f:(float)x)
#define SCALE_Y(y)		((flags & VPT_STRETCH)?((float)y)*(float)SCREENHEIGHT/200.0f:(float)y)

void gld_DrawNumPatch(int x, int y, int lump, int cm, enum patch_translation_e flags)
{ 
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  float width,height;
  float xpos, ypos;

  if (flags & VPT_TRANS)
  {
    gltexture=gld_RegisterPatch(lump,cm);
    gld_BindPatch(gltexture, cm);
  }
  else
  {
    gltexture=gld_RegisterPatch(lump,CR_DEFAULT);
    gld_BindPatch(gltexture, CR_DEFAULT);
  }
  if (!gltexture)
    return;
  fV1=0.0f;
  fV2=(float)gltexture->height/(float)gltexture->tex_height;
  if (flags & VPT_FLIP)
  {
    fU1=(float)gltexture->width/(float)gltexture->tex_width;
    fU2=0.0f;
  }
  else
  {
    fU1=0.0f;
    fU2=(float)gltexture->width/(float)gltexture->tex_width;
  }
  xpos=SCALE_X(x-gltexture->leftoffset);
  ypos=SCALE_Y(y-gltexture->topoffset);
  width=SCALE_X(gltexture->realtexwidth);
  height=SCALE_Y(gltexture->realtexheight);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(fU1, fV1); glVertex2f((xpos),(ypos));
		glTexCoord2f(fU1, fV2); glVertex2f((xpos),(ypos+height));
		glTexCoord2f(fU2, fV1); glVertex2f((xpos+width),(ypos));
		glTexCoord2f(fU2, fV2); glVertex2f((xpos+width),(ypos+height));
	glEnd();
}

void gld_DrawPatchFromMem(int x, int y, const patch_t *patch, int cm, enum patch_translation_e flags)
{ 
  extern int gld_GetTexDimension(int value);
  extern void gld_AddPatchToTexture(GLTexture *gltexture, unsigned char *buffer, const patch_t *patch, int originx, int originy, int cm);

  GLTexture *gltexture;
  unsigned char *buffer;
  float fU1,fU2,fV1,fV2;
  float width,height;
  float xpos, ypos;

  gltexture=(GLTexture *)GLMalloc(sizeof(GLTexture));
  if (!gltexture)
    return;
  gltexture->realtexwidth=patch->width;
  gltexture->realtexheight=patch->height;
  gltexture->leftoffset=patch->leftoffset;
  gltexture->topoffset=patch->topoffset;
  gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
  gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
  gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
  gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
  gltexture->buffer_width=gltexture->tex_width;
  gltexture->buffer_height=gltexture->tex_height;
#ifdef USE_GLU_IMAGESCALE
  gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
  gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
  gltexture->buffer_width=max(gltexture->realtexwidth, gltexture->tex_width);
  gltexture->buffer_height=max(gltexture->realtexheight, gltexture->tex_height);
#endif
  gltexture->buffer_size=gltexture->buffer_width*gltexture->buffer_height*4;
  if (gltexture->realtexwidth>gltexture->buffer_width)
    return;
  if (gltexture->realtexheight>gltexture->buffer_height)
    return;
  buffer=(unsigned char*)GLMalloc(gltexture->buffer_size);
  memset(buffer,0,gltexture->buffer_size);
  gld_AddPatchToTexture(gltexture, buffer, patch, 0, 0, cm);
	glGenTextures(1,&gltexture->glTexID[cm]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
#ifdef USE_GLU_IMAGESCALE
  if ((gltexture->buffer_width>gltexture->tex_width) ||
      (gltexture->buffer_height>gltexture->tex_height)
     )
  {
    unsigned char *scaledbuffer;

    scaledbuffer=(unsigned char*)GLMalloc(gltexture->tex_width*gltexture->tex_height*4);
    if (scaledbuffer)
    {
      gluScaleImage(GL_RGBA,
                    gltexture->buffer_width, gltexture->buffer_height,
                    GL_UNSIGNED_BYTE,buffer,
                    gltexture->tex_width, gltexture->tex_height,
                    GL_UNSIGNED_BYTE,scaledbuffer);
      GLFree(buffer);
      buffer=scaledbuffer;
      glTexImage2D( GL_TEXTURE_2D, 0, 4,
                    gltexture->tex_width, gltexture->tex_height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
  }
  else
#endif /* USE_GLU_IMAGESCALE */
  {
    glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                  gltexture->buffer_width, gltexture->buffer_height,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  }
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GLFree(buffer);

  fV1=0.0f;
  fV2=(float)gltexture->height/(float)gltexture->tex_height;
  if (flags & VPT_FLIP)
  {
    fU1=(float)gltexture->width/(float)gltexture->tex_width;
    fU2=0.0f;
  }
  else
  {
    fU1=0.0f;
    fU2=(float)gltexture->width/(float)gltexture->tex_width;
  }
  xpos=SCALE_X(x-gltexture->leftoffset);
  ypos=SCALE_Y(y-gltexture->topoffset);
  width=SCALE_X(gltexture->realtexwidth);
  height=SCALE_Y(gltexture->realtexheight);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(fU1, fV1); glVertex2f((xpos),(ypos));
		glTexCoord2f(fU1, fV2); glVertex2f((xpos),(ypos+height));
		glTexCoord2f(fU2, fV1); glVertex2f((xpos+width),(ypos));
		glTexCoord2f(fU2, fV2); glVertex2f((xpos+width),(ypos+height));
	glEnd();

	glDeleteTextures(1,&gltexture->glTexID[cm]);
  GLFree(gltexture);
}

#undef SCALE_X
#undef SCALE_Y

void gld_DrawBackground(const char* name)
{
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  int width,height;

  gltexture=gld_RegisterFlat(R_FlatNumForName(name), false);
  gld_BindFlat(gltexture);
  if (!gltexture)
    return;
  fU1=0;
  fV1=0;
  fU2=(float)SCREENWIDTH/(float)gltexture->realtexwidth;
  fV2=(float)SCREENHEIGHT/(float)gltexture->realtexheight;
  width=SCREENWIDTH;
  height=SCREENHEIGHT;
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(fU1, fV1); glVertex2f((float)(0),(float)(0));
		glTexCoord2f(fU1, fV2); glVertex2f((float)(0),(float)(0+height));
		glTexCoord2f(fU2, fV1); glVertex2f((float)(0+width),(float)(0));
		glTexCoord2f(fU2, fV2); glVertex2f((float)(0+width),(float)(0+height));
	glEnd();
}

void gld_DrawLine(int x0, int y0, int x1, int y1, byte BaseColor)
{
  const unsigned char *playpal=W_CacheLumpName("PLAYPAL");

	glDisable(GL_TEXTURE_2D);
	glColor3f((float)playpal[3*BaseColor]/255.0f,
			      (float)playpal[3*BaseColor+1]/255.0f,
			      (float)playpal[3*BaseColor+2]/255.0f);
	glBegin(GL_LINES);
		glVertex2i( x0, y0 );
		glVertex2i( x1, y1 );
	glEnd();
	glEnable(GL_TEXTURE_2D);
  W_UnlockLumpName("PLAYPAL");
}

void gld_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel)
{
  GLTexture *gltexture;
  float fU1,fU2,fV1,fV2;
  int x1,y1,x2,y2;
  float scale;
  float light;

  gltexture=gld_RegisterPatch(firstspritelump+weaponlump, CR_DEFAULT);
  if (!gltexture)
    return;
  gld_BindPatch(gltexture, CR_DEFAULT);
  fU1=0;
  fV1=0;
  fU2=(float)gltexture->width/(float)gltexture->tex_width;
  fV2=(float)gltexture->height/(float)gltexture->tex_height;
  x1=viewwindowx+vis->x1;
  x2=viewwindowx+vis->x2;
  scale=((float)vis->scale/(float)FRACUNIT);
  y1=viewwindowy+centery-(int)(((float)vis->texturemid/(float)FRACUNIT)*scale);
  y2=y1+(int)((float)gltexture->realtexheight*scale)+1;
  light=gld_CalcLightLevel(lightlevel);

  if (viewplayer->mo->flags & MF_SHADOW)
  {
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL,0.1f);
    //glColor4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
    glColor4f(0.2f,0.2f,0.2f,0.33f);
  }
  else
  {
		if (viewplayer->mo->flags & MF_TRANSLUCENT)
      gld_StaticLightAlpha(light,(float)tran_filter_pct/100.0f);
    else
  		gld_StaticLight(light);
  }
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(fU1, fV1); glVertex2f((float)(x1),(float)(y1));
		glTexCoord2f(fU1, fV2); glVertex2f((float)(x1),(float)(y2));
		glTexCoord2f(fU2, fV1); glVertex2f((float)(x2),(float)(y1));
		glTexCoord2f(fU2, fV2); glVertex2f((float)(x2),(float)(y2));
	glEnd();
  if(viewplayer->mo->flags & MF_SHADOW)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL,0.5f);
  }
  glColor3f(1.0f,1.0f,1.0f);
}

void gld_FillBlock(int x, int y, int width, int height, int col)
{
  const unsigned char *playpal=W_CacheLumpName("PLAYPAL");

	glDisable(GL_TEXTURE_2D);
	glColor3f((float)playpal[3*col]/255.0f,
			      (float)playpal[3*col+1]/255.0f,
			      (float)playpal[3*col+2]/255.0f);
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2i( x, y );
		glVertex2i( x, y+height );
		glVertex2i( x+width, y );
		glVertex2i( x+width, y+height );
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f,1.0f,1.0f);
  W_UnlockLumpName("PLAYPAL");
}

void gld_SetPalette(int palette)
{
  extra_red=0.0f;
  extra_green=0.0f;
  extra_blue=0.0f;
  extra_alpha=0.0f;
  if (palette>0)
  {
    if (palette<=8)
    {
      extra_red=(float)palette/2.0f;
      extra_green=0.0f;
      extra_blue=0.0f;
      extra_alpha=(float)palette/10.0f;
    }
    else
      if (palette<=12)
      {
        palette=palette-8;
        extra_red=(float)palette*1.0f;
        extra_green=(float)palette*0.8f;
        extra_blue=(float)palette*0.1f;
        extra_alpha=(float)palette/11.0f;
      }
      else
        if (palette==13)
        {
          extra_red=0.4f;
          extra_green=1.0f;
          extra_blue=0.0f;
          extra_alpha=0.2f;
        }
  }
  if (extra_red>1.0f)
    extra_red=1.0f;
  if (extra_green>1.0f)
    extra_green=1.0f;
  if (extra_blue>1.0f)
    extra_blue=1.0f;
  if (extra_alpha>1.0f)
    extra_alpha=1.0f;
}

void gld_ReadScreen (byte* scr)
{
  glReadPixels(0,0,SCREENWIDTH,SCREENHEIGHT,GL_RGB,GL_UNSIGNED_BYTE,scr);
}

GLvoid gld_Set2DMode()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(
    (GLdouble) 0,
		(GLdouble) SCREENWIDTH, 
		(GLdouble) SCREENHEIGHT, 
		(GLdouble) 0,
		(GLdouble) -1.0, 
		(GLdouble) 1.0 
  );
	glDisable(GL_DEPTH_TEST);
}

void gld_InitDrawScene(void)
{
  if (use_maindisplay_list)
  {
    if (gld_DisplayList==0)
      gld_DisplayList=glGenLists(1);
    if (gld_DisplayList)
      glNewList(gld_DisplayList,GL_COMPILE);
  }
}

void gld_Finish()
{
  if (use_maindisplay_list)
    if (gld_DisplayList)
    {
      glEndList();
      glCallList(gld_DisplayList);
    }
  gld_Set2DMode();
  glFinish();
	SDL_GL_SwapBuffers();
}

/*****************
 *               *
 * structs       *
 *               *
 *****************/

/* GLLoopDef is the struct for one loop. A loop is a list of vertexes
 * for triangles, which is calculated by the gluTesselator in gld_PrecalculateSector
 * and in gld_PreprocessCarvedFlat
 */
typedef struct
{
  float u,v;
  float x,y;
} GLVertex;

typedef struct
{
  GLenum mode; // GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN
  int vertexcount; // number of vertexes in this loop
  GLVertex *gl_vertexes; // list of pointers to Doom's vertexes
} GLLoopDef;

// GLSector is the struct for a sector with a list of loops.

typedef struct
{
  int loopcount; // number of loops for this sector
  GLLoopDef *loops; // the loops itself
  GLuint gl_list;
} GLSector;

typedef struct
{
  GLLoopDef loop; // the loops itself
  GLuint gl_list;
} GLSubSector;

typedef struct
{
  float x1,x2;
  float z1,z2;
  float linelength;
} GLSeg;

GLSeg *gl_segs=NULL;

typedef struct
{
  int segnum;
  float ytop,ybottom;
  float ul,ur,vt,vb;
  float ou,ov;
  float light;
  float lineheight;
  boolean trans;
  boolean sky;
  boolean skyflip;
  float skyymid;
  float skyyaw;
  GLTexture *gltexture;
} GLWall;

typedef struct
{
  int sectornum;
  int subsectornum;
  boolean ceiling;
  float light; // the lightlevel of the flat
  float uoffs,voffs; // the texture coordinates
  float z; // the z position of the flat (height)
  GLTexture *gltexture;
} GLFlat;

typedef struct
{
  int cm;
  boolean shadow;
  boolean trans;
  float x,y,z;
  float vt,vb;
  float ul,ur;
  float w,h;
  float voff,hoff;
	float light;
  fixed_t scale;
  GLTexture *gltexture;
} GLSprite;

typedef enum
{
  GLDIT_NONE,
  GLDIT_WALL,
  GLDIT_FLAT,
  GLDIT_SPRITE
} GLDrawItemType;

typedef struct
{
  GLDrawItemType itemtype;
  int itemcount;
  int firstitemindex;
} GLDrawItem;

typedef struct
{
  GLWall *walls;
  int num_walls;
  int max_walls;
  GLFlat *flats;
  int num_flats;
  int max_flats;
  GLSprite *sprites;
  int num_sprites;
  int max_sprites;
  GLDrawItem *drawitems;
  int num_drawitems;
  int max_drawitems;
} GLDrawInfo;

static GLDrawInfo gld_drawinfo;

// this is the list for all sectors to the loops
static GLSector *sectorloops;
// this is the list for all subsectors
static GLSubSector *subsectorloops;

byte rendermarker=0;
static byte *sectorrendered; // true if sector rendered (only here for malloc)
static byte *subsectorrendered; // true if subsector rendered (only here for malloc)
static byte *segrendered; // true if sector rendered (only here for malloc)

static FILE *levelinfo;

/*****************************
 *
 * FLATS
 *
 *****************************/

/* proff - 05/15/2000
 * The idea and algorithm to compute the flats with nodes and subsectors is
 * originaly from JHexen. I have redone it.
 */

#define FIX2DBL(x)		((double)(x))
#define MAX_CC_SIDES	64

static boolean gld_PointOnSide(vertex_t *p, divline_t *d)
{
	// We'll return false if the point c is on the left side.
	return ((FIX2DBL(d->y)-FIX2DBL(p->y))*FIX2DBL(d->dx)-(FIX2DBL(d->x)-FIX2DBL(p->x))*FIX2DBL(d->dy) >= 0);
}

// Lines start-end and fdiv must intersect.
static void gld_CalcIntersectionVertex(vertex_t *s, vertex_t *e, divline_t *d, vertex_t *i)
{
	double ax = FIX2DBL(s->x), ay = FIX2DBL(s->y), bx = FIX2DBL(e->x), by = FIX2DBL(e->y);
	double cx = FIX2DBL(d->x), cy = FIX2DBL(d->y), dx = cx+FIX2DBL(d->dx), dy = cy+FIX2DBL(d->dy);
	double r = ((ay-cy)*(dx-cx)-(ax-cx)*(dy-cy)) / ((bx-ax)*(dy-cy)-(by-ay)*(dx-cx));
	i->x = (fixed_t)((double)s->x + r*((double)e->x-(double)s->x));
	i->y = (fixed_t)((double)s->y + r*((double)e->y-(double)s->y));
}

#undef FIX2DBL

// Returns a pointer to the list of points. It must be used.
static vertex_t *gld_FlatEdgeClipper(int *numpoints, vertex_t *points, int numclippers, divline_t *clippers)
{
	unsigned char	sidelist[MAX_CC_SIDES];
	int				i, k, num = *numpoints;

	// We'll clip the polygon with each of the divlines. The left side of
	// each divline is discarded.
	for(i=0; i<numclippers; i++)
	{
		divline_t *curclip = &clippers[i];

		// First we'll determine the side of each vertex. Points are allowed
		// to be on the line.
		for(k=0; k<num; k++)
			sidelist[k] = gld_PointOnSide(&points[k], curclip);
		
		for(k=0; k<num; k++)
		{
			int startIdx = k, endIdx = k+1;
			// Check the end index.
			if(endIdx == num) endIdx = 0;	// Wrap-around.
			// Clipping will happen when the ends are on different sides.
			if(sidelist[startIdx] != sidelist[endIdx])
			{
				vertex_t newvert;

        gld_CalcIntersectionVertex(&points[startIdx], &points[endIdx], curclip, &newvert);

				// Add the new vertex. Also modify the sidelist.
				points = (vertex_t*)GLRealloc(points,(++num)*sizeof(vertex_t));
				if(num >= MAX_CC_SIDES) I_Error("Too many points in carver.\n");

				// Make room for the new vertex.
				memmove(&points[endIdx+1], &points[endIdx],
					(num - endIdx-1)*sizeof(vertex_t));
				memcpy(&points[endIdx], &newvert, sizeof(newvert));

				memmove(&sidelist[endIdx+1], &sidelist[endIdx], num-endIdx-1);
				sidelist[endIdx] = 1;
				
				// Skip over the new vertex.
				k++;
			}
		}
		
		// Now we must discard the points that are on the wrong side.
		for(k=0; k<num; k++)
			if(!sidelist[k])
			{
				memmove(&points[k], &points[k+1], (num - k-1)*sizeof(vertex_t));
				memmove(&sidelist[k], &sidelist[k+1], num - k-1);
				num--;
				k--;
			}
	}
	// Screen out consecutive identical points.
	for(i=0; i<num; i++)
	{
		int previdx = i-1;
		if(previdx < 0) previdx = num - 1;
		if(points[i].x == points[previdx].x 
			&& points[i].y == points[previdx].y)
		{
			// This point (i) must be removed.
			memmove(&points[i], &points[i+1], sizeof(vertex_t)*(num-i-1));
			num--;
			i--;
		}
	}
	*numpoints = num;
	return points;
}

static void gld_FlatConvexCarver(int ssidx, int num, divline_t *list)
{
  subsector_t *ssec=&subsectors[ssidx];
	int numclippers = num+ssec->numlines;
	divline_t *clippers;
	int i, numedgepoints;
	vertex_t *edgepoints;
	
  clippers=(divline_t*)GLMalloc(numclippers*sizeof(divline_t));
  if (!clippers)
    return;
	for(i=0; i<num; i++)
	{
		clippers[i].x = list[num-i-1].x;
		clippers[i].y = list[num-i-1].y;
		clippers[i].dx = list[num-i-1].dx;
  	clippers[i].dy = list[num-i-1].dy;
  }
	for(i=num; i<numclippers; i++)
	{
		seg_t *seg = &segs[ssec->firstline+i-num];
		clippers[i].x = seg->v1->x;
		clippers[i].y = seg->v1->y;
		clippers[i].dx = seg->v2->x-seg->v1->x;
		clippers[i].dy = seg->v2->y-seg->v1->y;
	}

	// Setup the 'worldwide' polygon.
	numedgepoints = 4;
	edgepoints = (vertex_t*)GLMalloc(numedgepoints*sizeof(vertex_t));
	
	edgepoints[0].x = INT_MIN;
	edgepoints[0].y = INT_MAX;
	
	edgepoints[1].x = INT_MAX;
	edgepoints[1].y = INT_MAX;
	
	edgepoints[2].x = INT_MAX;
	edgepoints[2].y = INT_MIN;
	
	edgepoints[3].x = INT_MIN;
	edgepoints[3].y = INT_MIN;

	// Do some clipping, <snip> <snip>
	edgepoints = gld_FlatEdgeClipper(&numedgepoints, edgepoints, numclippers, clippers);
	
	if(!numedgepoints)
	{
		if (levelinfo) fprintf(levelinfo, "All carved away: subsector %i - sector %i\n", ssec-subsectors, ssec->sector->iSectorID);
	}
	else
	{
	  if(numedgepoints >= 3)
    {
      GLVertex *gl_vertexes;

      gl_vertexes=GLMalloc(numedgepoints*sizeof(GLVertex));
      if (gl_vertexes)
      {
        int currentsector=ssec->sector->iSectorID;

        sectorloops[ currentsector ].loopcount++;
        sectorloops[ currentsector ].loops=Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_LEVEL, 0);
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].mode=GL_TRIANGLE_FAN;
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=numedgepoints;
        sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].gl_vertexes=gl_vertexes;

	      for(i = 0;  i < numedgepoints; i++)
	      {
		      gl_vertexes[i].u = ( (float)edgepoints[i].x/(float)FRACUNIT)/64.0f;
		      gl_vertexes[i].v = (-(float)edgepoints[i].y/(float)FRACUNIT)/64.0f;
          gl_vertexes[i].x = -(float)edgepoints[i].x/(float)MAP_SCALE;
          gl_vertexes[i].y =  (float)edgepoints[i].y/(float)MAP_SCALE;
	      }
      }
    }
	}
	// We're done, free the edgepoints memory.
	GLFree(edgepoints);
  GLFree(clippers);
}

static void gld_CarveFlats(int bspnode, int numdivlines, divline_t *divlines, boolean *sectorclosed)
{
	node_t		*nod;
	divline_t	*childlist, *dl;
	int			childlistsize = numdivlines+1;
	
	// If this is a subsector we are dealing with, begin carving with the
	// given list.
	if(bspnode & NF_SUBSECTOR)
	{
		// We have arrived at a subsector. The divline list contains all
		// the partition lines that carve out the subsector.
		int ssidx = bspnode & (~NF_SUBSECTOR);
    if (!sectorclosed[subsectors[ssidx].sector->iSectorID])
  		gld_FlatConvexCarver(ssidx, numdivlines, divlines);
		return;
	}

	// Get a pointer to the node.
	nod = nodes + bspnode;

	// Allocate a new list for each child.
	childlist = (divline_t*)GLMalloc(childlistsize*sizeof(divline_t));

	// Copy the previous lines.
	if(divlines) memcpy(childlist,divlines,numdivlines*sizeof(divline_t));

	dl = childlist + numdivlines;
	dl->x = nod->x;
	dl->y = nod->y;
	// The right child gets the original line (LEFT side clipped).
	dl->dx = nod->dx;
	dl->dy = nod->dy;
	gld_CarveFlats(nod->children[0],childlistsize,childlist,sectorclosed);

	// The left side. We must reverse the line, otherwise the wrong
	// side would get clipped.
	dl->dx = -nod->dx;
	dl->dy = -nod->dy;
	gld_CarveFlats(nod->children[1],childlistsize,childlist,sectorclosed);

	// We are finishing with this node, free the allocated list.
	GLFree(childlist);
}

#ifdef USE_GLU_TESS

static int currentsector; // the sector which is currently tesselated

// ntessBegin
//
// called when the tesselation of a new loop starts

static void CALLBACK ntessBegin( GLenum type )
{
#ifdef _DEBUG
  if (levelinfo)
  {
    if (type==GL_TRIANGLES)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLES\n");
    else
    if (type==GL_TRIANGLE_FAN)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLE_FAN\n");
    else
    if (type==GL_TRIANGLE_STRIP)
      fprintf(levelinfo, "\t\tBegin: GL_TRIANGLE_STRIP\n");
    else
      fprintf(levelinfo, "\t\tBegin: unknown\n");
  }
#endif
  // increase loopcount for currentsector
  sectorloops[ currentsector ].loopcount++;
  // reallocate to get space for another loop
  // PU_LEVEL is used, so this gets freed before a new level is loaded
  sectorloops[ currentsector ].loops=Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_LEVEL, 0);
  // set initial values for current loop
  // currentloop is -> sectorloops[currentsector].loopcount-1
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].mode=type;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=0;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].gl_vertexes=NULL;
}

// ntessError
//
// called when the tesselation failes (DEBUG only)

static void CALLBACK ntessError(GLenum error)
{
#ifdef _DEBUG
  const GLubyte *estring;
  estring = gluErrorString(error);
  fprintf(levelinfo, "\t\tTessellation Error: %s\n", estring);
#endif
}

// ntessCombine
//
// called when the two or more vertexes are on the same coordinate

static void CALLBACK ntessCombine( GLdouble coords[3], vertex_t *vert[4], GLfloat w[4], void **dataOut )
{
#ifdef _DEBUG
  if (levelinfo)
  {
    fprintf(levelinfo, "\t\tVertexCombine Coords: x %10.5f, y %10.5f z %10.5f\n", coords[0], coords[1], coords[2]);
    if (vert[0]) fprintf(levelinfo, "\t\tVertexCombine Vert1 : x %10i, y %10i p %p\n", vert[0]->x>>FRACBITS, vert[0]->y>>FRACBITS, vert[0]);
    if (vert[1]) fprintf(levelinfo, "\t\tVertexCombine Vert2 : x %10i, y %10i p %p\n", vert[1]->x>>FRACBITS, vert[1]->y>>FRACBITS, vert[1]);
    if (vert[2]) fprintf(levelinfo, "\t\tVertexCombine Vert3 : x %10i, y %10i p %p\n", vert[2]->x>>FRACBITS, vert[2]->y>>FRACBITS, vert[2]);
    if (vert[3]) fprintf(levelinfo, "\t\tVertexCombine Vert4 : x %10i, y %10i p %p\n", vert[3]->x>>FRACBITS, vert[3]->y>>FRACBITS, vert[3]);
  }
#endif
  // just return the first vertex, because all vertexes are on the same coordinate
  *dataOut = vert[0];
}

// ntessVertex
//
// called when a vertex is found

static void CALLBACK ntessVertex( vertex_t *vert )
{
  GLVertex *gl_vertexes;
  int vertexcount;

#ifdef _DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tVertex : x %10i, y %10i\n", vert->x>>FRACBITS, vert->y>>FRACBITS);
#endif
  // set vertexcount and vertexes variables to the values for the current loop, so the
  // rest of this function isn't to complicated
  vertexcount=sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount;
  gl_vertexes=sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].gl_vertexes;

  // increase vertex count
  vertexcount++;
  // realloc memory to get space for new vertex
  gl_vertexes=Z_Realloc(gl_vertexes, vertexcount*sizeof(GLVertex),PU_LEVEL,0);
  // add the new vertex (vert is the second argument of gluTessVertex)
  gl_vertexes[vertexcount-1].u=( (float)vert->x/(float)FRACUNIT)/64.0f;
  gl_vertexes[vertexcount-1].v=(-(float)vert->y/(float)FRACUNIT)/64.0f;
  gl_vertexes[vertexcount-1].x=-(float)vert->x/(float)MAP_SCALE;
  gl_vertexes[vertexcount-1].y= (float)vert->y/(float)MAP_SCALE;

  // set values of the loop to new values
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount=vertexcount;
  sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].gl_vertexes=gl_vertexes;
}

// ntessEnd
//
// called when the tesselation of a the current loop ends (DEBUG only)

static void CALLBACK ntessEnd( void )
{
#ifdef _DEBUG
  if (levelinfo)
    fprintf(levelinfo, "\t\tEnd loopcount %i vertexcount %i\n", sectorloops[currentsector].loopcount, sectorloops[ currentsector ].loops[ sectorloops[currentsector].loopcount-1 ].vertexcount);
#endif
}

static void gld_PrepareSectorSpecialEffects(int num)
{
  int i;

  // the following is for specialeffects. see r_bsp.c in R_Subsector
  sectors[num].no_toptextures=true;
  sectors[num].no_bottomtextures=true;
  for (i=0; i<sectors[num].linecount; i++)
  {
    if ( (sectors[num].lines[i]->sidenum[0]>=0) &&
         (sectors[num].lines[i]->sidenum[1]>=0) )
    {
      if (sides[sectors[num].lines[i]->sidenum[0]].toptexture!=R_TextureNumForName("-"))
        sectors[num].no_toptextures=false;
      if (sides[sectors[num].lines[i]->sidenum[0]].bottomtexture!=R_TextureNumForName("-"))
        sectors[num].no_bottomtextures=false;
      if (sides[sectors[num].lines[i]->sidenum[1]].toptexture!=R_TextureNumForName("-"))
        sectors[num].no_toptextures=false;
      if (sides[sectors[num].lines[i]->sidenum[1]].bottomtexture!=R_TextureNumForName("-"))
        sectors[num].no_bottomtextures=false;
    }
    else
    {
      sectors[num].no_toptextures=false;
      sectors[num].no_bottomtextures=false;
    }
  }
#ifdef _DEBUG
  if (sectors[num].no_toptextures)
    lprintf(LO_INFO,"Sector %i has no toptextures\n",num);
  if (sectors[num].no_bottomtextures)
    lprintf(LO_INFO,"Sector %i has no bottomtextures\n",num);
#endif
}

// gld_PrecalculateSector
//
// this calculates the loops for the sector "num"
//
// how does it work?
// first I have to credit Michael 'Kodak' Ryssen for the usage of the
// glu tesselation functions. the rest of this stuff is entirely done by me (proff).
// if there are any similarities, then they are implications of the algorithm.
//
// I'm starting with the first line of the current sector. I take it's ending vertex and
// add it to the tesselator. the current line is marked as used. then I'm searching for
// the next line which connects to the current line. if there is more than one line, I
// choose the one with the smallest angle to the current. if there is no next line, I
// start a new loop and take the first unused line in the sector. after all lines are
// processed, the polygon is tesselated.

static void gld_PrecalculateSector(int num)
{
  int i;
  boolean *lineadded=NULL;
  int linecount;
  int currentline;
  int oldline;
  int currentloop;
  int bestline;
  int bestlinecount;
  vertex_t *startvertex;
  vertex_t *currentvertex;
  angle_t lineangle;
  angle_t angle;
  angle_t bestangle;
  sector_t *backsector;
  GLUtesselator *tess;
  double *v=NULL;
  int maxvertexnum;
  int vertexnum;

  currentsector=num;
  lineadded=GLRealloc(lineadded,sectors[num].linecount*sizeof(boolean));
  if (!lineadded)
  {
    if (levelinfo) fclose(levelinfo);
    return;
  }
  // init tesselator
  tess=gluNewTess();
  if (!tess)
  {
    if (levelinfo) fclose(levelinfo);
    GLFree(lineadded);
    return;
  }
  // set callbacks
  gluTessCallback(tess, GLU_TESS_BEGIN, ntessBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX, ntessVertex);
  gluTessCallback(tess, GLU_TESS_ERROR, ntessError);
  gluTessCallback(tess, GLU_TESS_COMBINE, ntessCombine);
  gluTessCallback(tess, GLU_TESS_END, ntessEnd);
  if (levelinfo) fprintf(levelinfo, "sector %i, %i lines in sector\n", num, sectors[num].linecount);
  // remove any line which has both sides in the same sector (i.e. Doom2 Map01 Sector 1)
  for (i=0; i<sectors[num].linecount; i++)
  {
    lineadded[i]=false;
    if (sectors[num].lines[i]->sidenum[0]>=0)
      if (sectors[num].lines[i]->sidenum[1]>=0)
        if (sides[sectors[num].lines[i]->sidenum[0]].sector
          ==sides[sectors[num].lines[i]->sidenum[1]].sector)
        {
          lineadded[i]=true;
          if (levelinfo) fprintf(levelinfo, "line %4i (iLineID %4i) has both sides in same sector (removed)\n", i, sectors[num].lines[i]->iLineID);
        }
  }

  // initialize variables
  linecount=sectors[num].linecount;
  oldline=0;
  currentline=0;
  startvertex=sectors[num].lines[currentline]->v2;
  currentloop=0;
  vertexnum=0;
  maxvertexnum=0;
  // start tesselator
  if (levelinfo) fprintf(levelinfo, "gluTessBeginPolygon\n");
  gluTessBeginPolygon(tess, NULL);
  if (levelinfo) fprintf(levelinfo, "\tgluTessBeginContour\n");
  gluTessBeginContour(tess);
  while (linecount)
  {
    // if there is no connected line, then start new loop
    if ((oldline==currentline) || (startvertex==currentvertex))
    {
      currentline=-1;
      for (i=0; i<sectors[num].linecount; i++)
        if (!lineadded[i])
        {
          currentline=i;
          currentloop++;
          if ((sectors[num].lines[currentline]->sidenum[0]!=-1) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector==&sectors[num]) : false)
            startvertex=sectors[num].lines[currentline]->v1;
          else
            startvertex=sectors[num].lines[currentline]->v2;
          if (levelinfo) fprintf(levelinfo, "\tNew Loop %3i\n", currentloop);
          if (oldline!=0)
          {
            if (levelinfo) fprintf(levelinfo, "\tgluTessEndContour\n");
            gluTessEndContour(tess);
//            if (levelinfo) fprintf(levelinfo, "\tgluNextContour\n");
//            gluNextContour(tess, GLU_CW);
            if (levelinfo) fprintf(levelinfo, "\tgluTessBeginContour\n");
            gluTessBeginContour(tess);
          }
          break;
        }
    }
    if (currentline==-1)
      break;
    // add current line
    lineadded[currentline]=true;
    // check if currentsector is on the front side of the line ...
    if ((sectors[num].lines[currentline]->sidenum[0]!=-1) ? (sides[sectors[num].lines[currentline]->sidenum[0]].sector==&sectors[num]) : false)
    {
      // v2 is ending vertex
      currentvertex=sectors[num].lines[currentline]->v2;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v1->x,sectors[num].lines[currentline]->v1->y,sectors[num].lines[currentline]->v2->x,sectors[num].lines[currentline]->v2->y);
      lineangle=(lineangle>>ANGLETOFINESHIFT)*360/8192;
      if (lineangle>=180)
        lineangle=lineangle-360;
      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped false\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    else // ... or on the back side
    {
      // v1 is ending vertex
      currentvertex=sectors[num].lines[currentline]->v1;
      // calculate the angle of this line for use below
      lineangle = R_PointToAngle2(sectors[num].lines[currentline]->v2->x,sectors[num].lines[currentline]->v2->y,sectors[num].lines[currentline]->v1->x,sectors[num].lines[currentline]->v1->y);
      lineangle=(lineangle>>ANGLETOFINESHIFT)*360/8192;
      if (lineangle>=180)
        lineangle=lineangle-360;
      if (levelinfo) fprintf(levelinfo, "\t\tAdded Line %4i to Loop, iLineID %5i, Angle: %4i, flipped true\n", currentline, sectors[num].lines[currentline]->iLineID, lineangle);
    }
    if (vertexnum>=maxvertexnum)
    {
      maxvertexnum+=512;
      v=GLRealloc(v,maxvertexnum*3*sizeof(double));
    }
    // calculate coordinates for the glu tesselation functions
    v[vertexnum*3+0]=-(double)currentvertex->x/(double)MAP_SCALE;
    v[vertexnum*3+1]=0.0;
    v[vertexnum*3+2]= (double)currentvertex->y/(double)MAP_SCALE;
    // add the vertex to the tesselator, currentvertex is the pointer to the vertexlist of doom
    // v[vertexnum] is the GLdouble array of the current vertex
    if (levelinfo) fprintf(levelinfo, "\t\tgluTessVertex(%i, %i)\n",currentvertex->x>>FRACBITS,currentvertex->y>>FRACBITS);
    gluTessVertex(tess, &v[vertexnum*3], currentvertex);
    // increase vertexindex
    vertexnum++;
    // decrease linecount of current sector
    linecount--;
    // find the next line
    oldline=currentline; // if this isn't changed at the end of the search, a new loop will start
    bestline=-1; // set to start values
    bestlinecount=0;
    // set backsector if there is one
    if (sectors[num].lines[currentline]->sidenum[1]!=-1)
      backsector=sides[sectors[num].lines[currentline]->sidenum[1]].sector;
    else
      backsector=NULL;
    // search through all lines of the current sector
    for (i=0; i<sectors[num].linecount; i++)
      if (!lineadded[i]) // if the line isn't already added ...
        // check if one of the vertexes is the same as the current vertex
        if ((sectors[num].lines[i]->v1==currentvertex) || (sectors[num].lines[i]->v2==currentvertex))
        {
          // calculate the angle of this best line candidate
          if ((sectors[num].lines[i]->sidenum[0]!=-1) ? (sides[sectors[num].lines[i]->sidenum[0]].sector==&sectors[num]) : false)
            angle = R_PointToAngle2(sectors[num].lines[i]->v1->x,sectors[num].lines[i]->v1->y,sectors[num].lines[i]->v2->x,sectors[num].lines[i]->v2->y);
          else
            angle = R_PointToAngle2(sectors[num].lines[i]->v2->x,sectors[num].lines[i]->v2->y,sectors[num].lines[i]->v1->x,sectors[num].lines[i]->v1->y);
          angle=(angle>>ANGLETOFINESHIFT)*360/8192;
          if (angle>=180)
            angle=angle-360;
          // check if line is flipped ...
          if ((sectors[num].lines[i]->sidenum[0]!=-1) ? (sides[sectors[num].lines[i]->sidenum[0]].sector==&sectors[num]) : false)
          {
            // when the line is not flipped and startvertex is not the currentvertex then skip this line
            if (sectors[num].lines[i]->v1!=currentvertex)
              continue;
          }
          else
          {
            // when the line is flipped and endvertex is not the currentvertex then skip this line
            if (sectors[num].lines[i]->v2!=currentvertex)
              continue;
          }
          // set new best line candidate
          if (bestline==-1) // if this is the first one ...
          {
            bestline=i;
            bestangle=lineangle-angle;
            bestlinecount++;
          }
          else
            // check if the angle between the current line and this best line candidate is smaller then
            // the angle of the last candidate
            if (abs(lineangle-angle)<abs(bestangle))
            {
              bestline=i;
              bestangle=lineangle-angle;
              bestlinecount++;
            }
        }
    if (bestline!=-1) // if a line is found, make it the current line
    {
      currentline=bestline;
      if (bestlinecount>1)
        if (levelinfo) fprintf(levelinfo, "\t\tBestlinecount: %4i\n", bestlinecount);
    }
  }
  // let the tesselator calculate the loops
  if (levelinfo) fprintf(levelinfo, "\tgluTessEndContour\n");
  gluTessEndContour(tess);
  if (levelinfo) fprintf(levelinfo, "gluTessEndPolygon\n");
  gluTessEndPolygon(tess);
  // clean memory
  gluDeleteTess(tess);
  GLFree(v);
  GLFree(lineadded);
}

#endif /* USE_GLU_TESS */

/********************************************
 * Name     : gld_GetSubSectorVertices	    *
 * created  : 08/13/00					            *
 * modified : 09/18/00, adapted for PrBoom  *
 * author   : figgi						              *
 * what		  : prepares subsectorvertices    *
 *            (glnodes only)			          *
 ********************************************/

void gld_GetSubSectorVertices(void)
{
	int			 i, j;
  int			 numedgepoints;
	subsector_t* ssector;
	GLVertex*    vtx;
		
	for(i = 0; i < numsubsectors; i++)
	{
		ssector = &subsectors[i];

		numedgepoints  = ssector->numlines;

 		vtx = Z_Malloc(sizeof(GLVertex) * numedgepoints, PU_LEVEL, 0);	

    if (vtx)
    {
/*
			int currentsector = ssector->sector->iSectorID;

			sectorloops[currentsector].loopcount++;
			sectorloops[currentsector].loops = Z_Realloc(sectorloops[currentsector].loops,sizeof(GLLoopDef)*sectorloops[currentsector].loopcount, PU_STATIC, 0);
			sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].mode		 = GL_TRIANGLE_FAN;
			sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].vertexcount = numedgepoints;
			sectorloops[currentsector].loops[sectorloops[currentsector].loopcount-1].gl_vertexes = vtx;
*/
#ifdef _DEBUG
      if (subsectorloops[ i ].loop.gl_vertexes)
        lprintf(LO_INFO,"subsector %i already carved\n",i);
#endif

      subsectorloops[ i ].loop.mode=GL_TRIANGLE_FAN;
      subsectorloops[ i ].loop.vertexcount=numedgepoints;
      subsectorloops[ i ].loop.gl_vertexes=vtx;

			for(j = 0;  j < numedgepoints; j++)
			{
        vtx[j].x = -(float)(segs[ssector->firstline + j].v1->x)/MAP_SCALE;
        vtx[j].y =  (float)(segs[ssector->firstline + j].v1->y)/MAP_SCALE;
        vtx[j].u =( (float)(segs[ssector->firstline + j].v1->x)/FRACUNIT)/64.0f;
        vtx[j].v =(-(float)(segs[ssector->firstline + j].v1->y)/FRACUNIT)/64.0f;
			}
	  }
  }
}

// gld_PreprocessLevel
//
// this checks all sectors if they are closed and calls gld_PrecalculateSector to
// calculate the loops for every sector
// the idea to check for closed sectors is from DEU. check next commentary
/*
      Note from RQ:
      This is a very simple idea, but it works!  The first test (above)
      checks that all Sectors are closed.  But if a closed set of LineDefs
      is moved out of a Sector and has all its "external" SideDefs pointing
      to that Sector instead of the new one, then we need a second test.
      That's why I check if the SideDefs facing each other are bound to
      the same Sector.
      
      Other note from RQ:
      Nowadays, what makes the power of a good editor is its automatic tests.
      So, if you are writing another Doom editor, you will probably want
      to do the same kind of tests in your program.  Fine, but if you use
      these ideas, don't forget to credit DEU...  Just a reminder... :-)
*/
// so I credited DEU

void gld_PreprocessSectors(void)
{
  boolean *sectorclosed;
#ifdef USE_GLU_TESS // figgi
  int i;
  char *vertexcheck;
  int v1num;
  int v2num;
  int j;
#endif

#ifdef _DEBUG
  levelinfo=fopen("levelinfo.txt","a");
  if (levelinfo)
  {
    if (gamemode==commercial)
      fprintf(levelinfo,"MAP%02i\n",gamemap);
    else
      fprintf(levelinfo,"E%iM%i\n",gameepisode,gamemap);
  }
#endif

  sectorclosed=GLMalloc(numsectors*sizeof(boolean));
  if (!sectorclosed)
    I_Error("Not enough memory for array (sectorclosed)\n");
  memset(sectorclosed, 0, sizeof(boolean)*numsectors);

  GLFree(sectorloops);
  sectorloops=GLMalloc(sizeof(GLSector)*numsectors);
  if (!sectorloops)
    I_Error("Not enough memory for array (sectorloops)\n");
  memset(sectorloops, 0, sizeof(GLSector)*numsectors);

  GLFree(sectorrendered);
  sectorrendered=GLMalloc(numsectors*sizeof(byte));
  if (!sectorrendered)
    I_Error("Not enough memory for array (sectorrendered)\n");
  memset(sectorrendered, 0, numsectors*sizeof(byte));

  GLFree(subsectorloops);
  subsectorloops=GLMalloc(sizeof(GLSubSector)*numsubsectors);
  if (!subsectorloops)
    I_Error("Not enough memory for array (subsectorloops)\n");
  memset(subsectorloops, 0, sizeof(GLSubSector)*numsubsectors);

  GLFree(subsectorrendered);
  subsectorrendered=GLMalloc(numsubsectors*sizeof(byte));
  if (!subsectorrendered)
    I_Error("Not enough memory for array (subsectorrendered)\n");
  memset(subsectorrendered, 0, numsubsectors*sizeof(byte));

  GLFree(segrendered);
  segrendered=GLMalloc(numsegs*sizeof(byte));
  if (!segrendered)
    I_Error("Not enough memory for array (segrendered)\n");
  memset(segrendered, 0, numsegs*sizeof(byte));

#ifdef USE_GLU_TESS
  if (usingGLNodes == false)
  {
    vertexcheck=GLMalloc(numvertexes*sizeof(char));
    if (!vertexcheck)
    {
      if (levelinfo) fclose(levelinfo);
      I_Error("Not enough memory for array (vertexcheck)\n");
      return;
    }

    for (i=0; i<numsectors; i++)
    {
      memset(vertexcheck,0,numvertexes*sizeof(char));
      for (j=0; j<sectors[i].linecount; j++)
      {
        v1num=((int)sectors[i].lines[j]->v1-(int)vertexes)/sizeof(vertex_t);
        v2num=((int)sectors[i].lines[j]->v2-(int)vertexes)/sizeof(vertex_t);
        if ((v1num>=numvertexes) || (v2num>=numvertexes))
          continue;
        if (sectors[i].lines[j]->sidenum[0]>=0)
          if (sides[sectors[i].lines[j]->sidenum[0]].sector==&sectors[i])
          {
            vertexcheck[v1num]|=1;
            vertexcheck[v2num]|=2;
          }
        if (sectors[i].lines[j]->sidenum[1]>=0)
          if (sides[sectors[i].lines[j]->sidenum[1]].sector==&sectors[i])
          {
            vertexcheck[v1num]|=2;
            vertexcheck[v2num]|=1;
          }
      }
      if (sectors[i].linecount<3)
      {
#ifdef _DEBUG
        lprintf(LO_ERROR, "sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
#endif
        if (levelinfo) fprintf(levelinfo, "sector %i is not closed! %i lines in sector\n", i, sectors[i].linecount);
        sectorclosed[i]=false;
      }
      else
      {
        sectorclosed[i]=true;
        for (j=0; j<numvertexes; j++)
        {
          if ((vertexcheck[j]==1) || (vertexcheck[j]==2))
          {
#ifdef _DEBUG
            lprintf(LO_ERROR, "sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
#endif
            if (levelinfo) fprintf(levelinfo, "sector %i is not closed at vertex %i ! %i lines in sector\n", i, j, sectors[i].linecount);
            sectorclosed[i]=false;
          }
        }
      }
	    // figgi -- adapted for glnodes
      if ( (sectorclosed[i]) && (usingGLNodes == false) )
        gld_PrecalculateSector(i);	
    }
    GLFree(vertexcheck);
  }
#endif /* USE_GLU_TESS */

  for (i=0; i<numsectors; i++)
    gld_PrepareSectorSpecialEffects(i);

  // figgi -- adapted for glnodes
  if (usingGLNodes == false)
	  gld_CarveFlats(numnodes-1, 0, 0, sectorclosed);
  else
	  gld_GetSubSectorVertices();

  if (levelinfo) fclose(levelinfo);
  GLFree(sectorclosed);
}

static float roll     = 0.0f;
static float yaw      = 0.0f;
static float inv_yaw  = 0.0f;
static float pitch    = 0.0f;

void gld_StartDrawScene(void)
{
  float trZ = -5.0f;
  float trY ;
  float xCamera,yCamera;

  extern int screenblocks;
  int height;

  if (screenblocks == 11)
    height = SCREENHEIGHT;
  else if (screenblocks == 10)
    height = SCREENHEIGHT;
  else
    height = (screenblocks*SCREENHEIGHT/10) & ~7;
  
 	glViewport(viewwindowx, SCREENHEIGHT-(height+viewwindowy-((height-viewheight)/2)), viewwidth, height);
	glScissor(viewwindowx, SCREENHEIGHT-(viewheight+viewwindowy), viewwidth, viewheight);
  glEnable(GL_SCISSOR_TEST);
	// Player coordinates
	xCamera=-(float)viewx/(float)MAP_SCALE;
	yCamera=(float)viewy/(float)MAP_SCALE;
	trY=(float)viewz/(float)MAP_SCALE;
	
	yaw=270.0f-(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;
	inv_yaw=-90.0f+(float)(viewangle>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;

#ifdef _DEBUG
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
	glClear(GL_DEPTH_BUFFER_BIT);
#endif

/*
  if (usingGLNodes)
    glDisable(GL_DEPTH_TEST);
  else
*/
    glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

//	gluPerspective(64.0f, 320.0f/200.0f, 0.01f, 2*fSkyRadius);
	gluPerspective(64.0f, 320.0f/200.0f, 0.05f, 64.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
  glRotatef(roll,  0.0f, 0.0f, 1.0f);
	glRotatef(pitch, 1.0f, 0.0f, 0.0f);
	glRotatef(yaw,   0.0f, 1.0f, 0.0f);
	glTranslatef(-xCamera, -trY, -yCamera);

  if (use_fog)
	  glEnable(GL_FOG);
  else
    glDisable(GL_FOG);
  rendermarker++;
  gld_drawinfo.num_walls=0;
  gld_drawinfo.num_flats=0;
  gld_drawinfo.num_sprites=0;
  gld_drawinfo.num_drawitems=0;
  if (gld_drawinfo.max_drawitems>0)
    memset(gld_drawinfo.drawitems,0,gld_drawinfo.max_drawitems*sizeof(GLDrawItem));
}

void gld_EndDrawScene(void)
{
  extern void R_DrawPlayerSprites (void);

	glDisable(GL_POLYGON_SMOOTH);

	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT); 
	glDisable(GL_FOG); 
	gld_Set2DMode();

	if (viewangleoffset <= 1024<<ANGLETOFINESHIFT || 
	 	viewangleoffset >=-1024<<ANGLETOFINESHIFT)
  {	// don't draw on side views
		R_DrawPlayerSprites ();
	}
  if (extra_alpha>0.0f)
  {
    glDisable(GL_ALPHA_TEST);
	  glColor4f(extra_red, extra_green, extra_blue, extra_alpha);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLE_STRIP);
  		glVertex2f( 0.0f, 0.0f);
	  	glVertex2f( 0.0f, (float)SCREENHEIGHT);
		  glVertex2f( (float)SCREENWIDTH, 0.0f);
		  glVertex2f( (float)SCREENWIDTH, (float)SCREENHEIGHT);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
  }

	glColor3f(1.0f,1.0f,1.0f);
  glDisable(GL_SCISSOR_TEST);
}

static void gld_AddDrawItem(GLDrawItemType itemtype, int itemindex)
{
  if (gld_drawinfo.num_drawitems>=gld_drawinfo.max_drawitems)
  {
    gld_drawinfo.max_drawitems+=64;
    gld_drawinfo.drawitems=Z_Realloc(gld_drawinfo.drawitems,gld_drawinfo.max_drawitems*sizeof(GLWall),PU_LEVEL,0);
    gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].itemtype=itemtype;
    gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].itemcount=1;
    gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].firstitemindex=itemindex;
    return;
  }
  if (gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].itemtype!=itemtype)
  {
    if (gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].itemtype!=GLDIT_NONE)
      gld_drawinfo.num_drawitems++;
    if (gld_drawinfo.num_drawitems>=gld_drawinfo.max_drawitems)
    {
      gld_drawinfo.max_drawitems+=64;
      gld_drawinfo.drawitems=Z_Realloc(gld_drawinfo.drawitems,gld_drawinfo.max_drawitems*sizeof(GLWall),PU_LEVEL,0);
    }
    gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].itemtype=itemtype;
    gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].itemcount=1;
    gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].firstitemindex=itemindex;
    return;
  }
  gld_drawinfo.drawitems[gld_drawinfo.num_drawitems].itemcount++;
}

/*****************
 *               *
 * Walls         *
 *               *
 *****************/

static void gld_DrawWall(GLWall *wall)
{
  GLSeg *glseg=&gl_segs[wall->segnum];

  if (wall->gltexture)
  {
    gld_BindTexture(wall->gltexture);
    if (wall->trans)
      gld_StaticLightAlpha(wall->light, (float)tran_filter_pct/100.0f);
    else
      gld_StaticLight(wall->light);
  }
  else
  {
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f,0.0f,0.0f,1.0f);
  }
  if (wall->sky)
  {
    if (wall->gltexture)
    {
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      glEnable(GL_TEXTURE_GEN_Q);
      glMatrixMode(GL_TEXTURE);
      glPushMatrix();
      if (wall->skyflip)
        glScalef(-128.0f/(float)wall->gltexture->buffer_width,200.0f/320.0f*2.0f,1.0f);
      else
        glScalef(128.0f/(float)wall->gltexture->buffer_width,200.0f/320.0f*2.0f,1.0f);
      glTranslatef(-2.0f*(wall->skyyaw/90.0f),200.0f/320.0f*(wall->skyymid/100.0f),0.0f);
    }
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glBegin(GL_TRIANGLE_STRIP);
      glVertex3f(glseg->x1,wall->ytop,glseg->z1);
      glVertex3f(glseg->x1,wall->ybottom,glseg->z1);
      glVertex3f(glseg->x2,wall->ytop,glseg->z2);
      glVertex3f(glseg->x2,wall->ybottom,glseg->z2);
    glEnd();
    if (wall->gltexture)
    {
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glDisable(GL_TEXTURE_GEN_Q);
      glDisable(GL_TEXTURE_GEN_T);
      glDisable(GL_TEXTURE_GEN_S);
    }
  }
  else
  {
    glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(wall->ul+wall->ou,wall->vt+wall->ov); glVertex3f(glseg->x1,wall->ytop,glseg->z1);
      glTexCoord2f(wall->ul+wall->ou,wall->vb+wall->ov); glVertex3f(glseg->x1,wall->ybottom,glseg->z1);
      glTexCoord2f(wall->ur+wall->ou,wall->vt+wall->ov); glVertex3f(glseg->x2,wall->ytop,glseg->z2);
      glTexCoord2f(wall->ur+wall->ou,wall->vb+wall->ov); glVertex3f(glseg->x2,wall->ybottom,glseg->z2);
    glEnd();
  }
  if (!wall->gltexture)
    glEnable(GL_TEXTURE_2D);
}

#define LINE seg->linedef
#define CALC_Y_VALUES(walldef, floor_height, ceiling_height)\
  (walldef).ytop=((float)(ceiling_height)/(float)MAP_SCALE)+0.001f;\
  (walldef).ybottom=((float)(floor_height)/(float)MAP_SCALE)-0.001f;\
  (walldef).lineheight=(float)fabs(((ceiling_height)/(float)FRACUNIT)-((floor_height)/(float)FRACUNIT))

#define CALC_TEX_VALUES_TOP(w, seg, peg)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width;\
  (w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->buffer_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->buffer_height)\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->buffer_height\
  )

#define CALC_TEX_VALUES_MIDDLE1S(w, seg, peg)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width;\
  (w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->buffer_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->buffer_height)\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->buffer_height\
  )

#define CALC_TEX_VALUES_MIDDLE2S(w, seg, peg)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width;\
  (w).ov=0.0f;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->buffer_width;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->buffer_height)\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->buffer_height\
  )

#define CALC_TEX_VALUES_BOTTOM(w, seg, peg, v_offset)\
  (w).ou=((float)((seg)->sidedef->textureoffset+(seg)->offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_width;\
  (w).ov=((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height;\
  (w).ul=0.0f;\
  (w).ur=(float)gl_segs[(w).segnum].linelength/(float)(w).gltexture->realtexwidth;\
  (peg)?\
  (\
    (w).vb=((float)(w).gltexture->height/(float)(w).gltexture->tex_height),\
    (w).vt=(w).vb-((float)(w).lineheight/(float)(w).gltexture->buffer_height),\
    (w).ov-=((float)(v_offset)/(float)FRACUNIT)/(float)(w).gltexture->buffer_height\
  ):(\
    (w).vt=0.0f,\
    (w).vb=(float)(w).lineheight/(float)(w).gltexture->buffer_height\
  )

#define SKYTEXTURE(sky1,sky2)\
  if ((sky1) & PL_SKYFLAT)\
  {\
	  const line_t *l = &lines[sky1 & ~PL_SKYFLAT];\
	  const side_t *s = *l->sidenum + sides;\
    wall.gltexture=gld_RegisterTexture(texturetranslation[s->toptexture], false);\
	  wall.skyyaw=-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;\
	  wall.skyymid = (float)s->rowoffset/(float)FRACUNIT - 28.0f;\
	  wall.skyflip = l->special==272 ? false : true;\
  }\
  else\
  if ((sky2) & PL_SKYFLAT)\
  {\
	  const line_t *l = &lines[sky2 & ~PL_SKYFLAT];\
	  const side_t *s = *l->sidenum + sides;\
    wall.gltexture=gld_RegisterTexture(texturetranslation[s->toptexture], false);\
	  wall.skyyaw=-(float)((viewangle+s->textureoffset)>>ANGLETOFINESHIFT)*360.0f/FINEANGLES;\
	  wall.skyymid = (float)s->rowoffset/(float)FRACUNIT - 28.0f;\
	  wall.skyflip = l->special==272 ? false : true;\
  }\
  else\
  {\
    wall.gltexture=gld_RegisterTexture(skytexture, false);\
	  wall.skyyaw=yaw+90.0f;\
	  wall.skyymid = 100.0f;\
	  wall.skyflip = false;\
  }

#define ADDWALL(wall)\
{\
  if (gld_drawinfo.num_walls>=gld_drawinfo.max_walls)\
  {\
    gld_drawinfo.max_walls+=128;\
    gld_drawinfo.walls=Z_Realloc(gld_drawinfo.walls,gld_drawinfo.max_walls*sizeof(GLWall),PU_LEVEL,0);\
  }\
  gld_AddDrawItem(GLDIT_WALL, gld_drawinfo.num_walls);\
  gld_drawinfo.walls[gld_drawinfo.num_walls++]=*wall;\
};

void gld_AddWall(seg_t *seg)
{
  GLWall wall;
  GLTexture *temptex;
  sector_t *frontsector;
  sector_t *backsector;
  sector_t ftempsec; // needed for R_FakeFlat
  sector_t btempsec; // needed for R_FakeFlat

  if (!segrendered)
    return;
  if (segrendered[seg->iSegID]==rendermarker)
    return;
  segrendered[seg->iSegID]=rendermarker;
  if (!seg->frontsector)
    return;
  frontsector=R_FakeFlat(seg->frontsector, &ftempsec, NULL, NULL, false); // for boom effects
  if (!frontsector)
    return;
  wall.segnum=seg->iSegID;

  wall.light=gld_CalcLightLevel(frontsector->lightlevel+(extralight<<5));
  wall.gltexture=NULL;
  wall.trans=0;

  if (!seg->backsector) /* onesided */
  {
    if (frontsector->ceilingpic==skyflatnum)
    {
      wall.ytop=255.0f;
      wall.ybottom=(float)frontsector->ceilingheight/(float)MAP_SCALE;
      SKYTEXTURE(frontsector->sky,frontsector->sky);
      wall.sky=true;
      ADDWALL(&wall);
    }
    if (frontsector->floorpic==skyflatnum)
    {
      wall.ytop=(float)frontsector->floorheight/(float)MAP_SCALE;
      wall.ybottom=-255.0f;
      SKYTEXTURE(frontsector->sky,frontsector->sky);
      wall.sky=true;
      ADDWALL(&wall);
    }
    temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true);
    if (temptex)
    {
      wall.gltexture=temptex;
      CALC_Y_VALUES(wall, frontsector->floorheight, frontsector->ceilingheight);
      CALC_TEX_VALUES_MIDDLE1S(wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0);
      wall.sky=false;
      ADDWALL(&wall);
    }
  }
  else /* twosided */
  {
    int floor_height,ceiling_height;

    backsector=R_FakeFlat(seg->backsector, &btempsec, NULL, NULL, true); // for boom effects
    if (!backsector)
      return;
    /* toptexture */
    ceiling_height=frontsector->ceilingheight;
    floor_height=backsector->ceilingheight;
    if (frontsector->ceilingpic==skyflatnum)
    {
      wall.ytop=255.0f;
      if (
          (backsector->ceilingheight==backsector->floorheight) &&
          (backsector->ceilingpic==skyflatnum)
         )
      {
        wall.ybottom=(float)backsector->floorheight/(float)MAP_SCALE;
        SKYTEXTURE(frontsector->sky,backsector->sky);
        wall.sky=true;
        ADDWALL(&wall);
      }
      else
      {
        if ( (texturetranslation[seg->sidedef->toptexture]!=R_TextureNumForName("-")) )
        {
          wall.ybottom=(float)frontsector->ceilingheight/(float)MAP_SCALE;
          SKYTEXTURE(frontsector->sky,backsector->sky);
          wall.sky=true;
          ADDWALL(&wall);
        }
        else
          //if ( backsector->ceilingpic != skyflatnum )
          if ( (backsector->ceilingheight <= frontsector->floorheight) ||
               (backsector->ceilingpic != skyflatnum) )
          {
            wall.ybottom=(float)backsector->ceilingheight/(float)MAP_SCALE;
            SKYTEXTURE(frontsector->sky,backsector->sky);
            wall.sky=true;
            ADDWALL(&wall);
          }
      }
    }
    if (floor_height<ceiling_height)
    {
      if (!((frontsector->ceilingpic==skyflatnum) && (backsector->ceilingpic==skyflatnum)))
      {
        temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->toptexture], true);
        if (temptex)
        {
          wall.gltexture=temptex;
          CALC_Y_VALUES(wall, floor_height, ceiling_height);
          CALC_TEX_VALUES_TOP(wall, seg, (LINE->flags & (ML_DONTPEGBOTTOM | ML_DONTPEGTOP))==0);
          wall.sky=false;
          ADDWALL(&wall);
        }
      }
    }
    
    /* midtexture */
    temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->midtexture], true);
    if (temptex)
    {
      wall.gltexture=temptex;
      if ( (LINE->flags & ML_DONTPEGBOTTOM) >0)
      {
        if (seg->backsector->ceilingheight<=seg->frontsector->floorheight)
          goto bottomtexture;
        floor_height=max(seg->frontsector->floorheight,seg->backsector->floorheight)+(seg->sidedef->rowoffset);
        ceiling_height=floor_height+(wall.gltexture->realtexheight<<FRACBITS);
      }
      else
      {
        if (seg->backsector->ceilingheight<=seg->frontsector->floorheight)
          goto bottomtexture;
        ceiling_height=min(seg->frontsector->ceilingheight,seg->backsector->ceilingheight)+(seg->sidedef->rowoffset);
        floor_height=ceiling_height-(wall.gltexture->realtexheight<<FRACBITS);
      }
      CALC_Y_VALUES(wall, floor_height, ceiling_height);
      CALC_TEX_VALUES_MIDDLE2S(wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0);
      if (seg->linedef->tranlump >= 0 && general_translucency)
        wall.trans=1;
      wall.sky=false;
      ADDWALL(&wall);
      wall.trans=0;
    }
bottomtexture:
    /* bottomtexture */
    ceiling_height=backsector->floorheight;
    floor_height=frontsector->floorheight;
    if (frontsector->floorpic==skyflatnum)
    {
      wall.ybottom=-255.0f;
      if (
          (backsector->ceilingheight==backsector->floorheight) &&
          (backsector->floorpic==skyflatnum)
         )
      {
        wall.ytop=(float)backsector->floorheight/(float)MAP_SCALE;
        SKYTEXTURE(frontsector->sky,backsector->sky);
        wall.sky=true;
        ADDWALL(&wall);
      }
      else
      {
        if ( (texturetranslation[seg->sidedef->toptexture]!=R_TextureNumForName("-")) )
        {
          wall.ytop=(float)frontsector->floorheight/(float)MAP_SCALE;
          SKYTEXTURE(frontsector->sky,backsector->sky);
          wall.sky=true;
          ADDWALL(&wall);
        }
        else
          //if ( backsector->floorpic != skyflatnum )
          if ( (backsector->floorheight >= frontsector->ceilingheight) ||
               (backsector->floorpic != skyflatnum) )
          {
            wall.ytop=(float)backsector->floorheight/(float)MAP_SCALE;
            SKYTEXTURE(frontsector->sky,backsector->sky);
            wall.sky=true;
            ADDWALL(&wall);
          }
      }
    }
    if (floor_height<ceiling_height)
    {
      temptex=gld_RegisterTexture(texturetranslation[seg->sidedef->bottomtexture], true);
      if (temptex)
      {
        wall.gltexture=temptex;
        CALC_Y_VALUES(wall, floor_height, ceiling_height);
        CALC_TEX_VALUES_BOTTOM(wall, seg, (LINE->flags & ML_DONTPEGBOTTOM)>0, floor_height-frontsector->ceilingheight);
        wall.sky=false;
        ADDWALL(&wall);
      }
    }
  }
}

#undef ADDWALL
#undef LINE
#undef FRONTSECTOR
#undef BACKSECTOR
#undef CALC_Y_VALUES
#undef CALC_TEX_VALUES_TOP
#undef CALC_TEX_VALUES_MIDDLE
#undef CALC_TEX_VALUES_BOTTOM
#undef SKYTEXTURE

static void gld_PreprocessSegs(void)
{
  int i;
  
  GLFree(gl_segs);
  gl_segs=GLMalloc(numsegs*sizeof(GLSeg));
  for (i=0; i<numsegs; i++)
  {
    gl_segs[i].x1=-(float)segs[i].v1->x/(float)MAP_SCALE;
    gl_segs[i].z1= (float)segs[i].v1->y/(float)MAP_SCALE;
    gl_segs[i].x2=-(float)segs[i].v2->x/(float)MAP_SCALE;
    gl_segs[i].z2= (float)segs[i].v2->y/(float)MAP_SCALE;
  	gl_segs[i].linelength = segs[i].length; 
  }
}

/*****************
 *               *
 * Flats         *
 *               *
 *****************/

static void gld_DrawFlat(GLFlat *flat)
{
  int loopnum; // current loop number
  GLLoopDef *currentloop; // the current loop
  int vertexnum; // the current vertexnumber
  GLVertex *currentvertex; // the current vertex

  // enable backside removing
  //glEnable(GL_CULL_FACE);
  if (!flat->ceiling) // if it is a floor ...
    glCullFace(GL_FRONT);
  else
    glCullFace(GL_BACK);
  gld_BindFlat(flat->gltexture);
  gld_StaticLight(flat->light);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0.0f,flat->z,0.0f);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glTranslatef(flat->uoffs/64.0f,flat->voffs/64.0f,0.0f);
  if (flat->sectornum>=0)
  {
    // go through all loops of this sector
    if (!sectorloops[flat->sectornum].gl_list)
    {
      if (use_display_lists)
      {
        sectorloops[flat->sectornum].gl_list=glGenLists(1);
        if (!sectorloops[flat->sectornum].gl_list)
        {
          glPopMatrix();
          glMatrixMode(GL_MODELVIEW);
          glPopMatrix();
          return;
        }
        glNewList(sectorloops[flat->sectornum].gl_list,GL_COMPILE_AND_EXECUTE);
      }
      for (loopnum=0; loopnum<sectorloops[flat->sectornum].loopcount; loopnum++)
      {
        // set the current loop
        currentloop=&sectorloops[flat->sectornum].loops[loopnum];
        if (!currentloop)
          continue;
        // set the mode (GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN)
        glBegin(currentloop->mode);
        // go through all vertexes of this loop
        for (vertexnum=0; vertexnum<currentloop->vertexcount; vertexnum++)
        {
          // set the current vertex
          currentvertex=&currentloop->gl_vertexes[vertexnum];
          if (!currentvertex)
            continue;
          // set texture coordinate of this vertex
          glTexCoord2f(currentvertex->u,currentvertex->v);
          // set vertex coordinate
          glVertex3f(currentvertex->x,0.0f,currentvertex->y);
        }
        // end of loop
        glEnd();
      }
      if (use_display_lists)
        glEndList();
    }
    else
      glCallList(sectorloops[flat->sectornum].gl_list);
  }
  if (flat->subsectornum>=0)
  {
    // go through all loops of this sector
    if (!subsectorloops[flat->subsectornum].gl_list)
    {
      if (use_display_lists)
      {
        subsectorloops[flat->subsectornum].gl_list=glGenLists(1);
        if (!subsectorloops[flat->subsectornum].gl_list)
        {
          glPopMatrix();
          glMatrixMode(GL_MODELVIEW);
          glPopMatrix();
          return;
        }
        glNewList(subsectorloops[flat->subsectornum].gl_list,GL_COMPILE_AND_EXECUTE);
      }
      // set the current loop
      currentloop=&subsectorloops[flat->subsectornum].loop;
      // set the mode (GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN)
      glBegin(currentloop->mode);
      // go through all vertexes of this loop
      for (vertexnum=0; vertexnum<currentloop->vertexcount; vertexnum++)
      {
        // set the current vertex
        currentvertex=&currentloop->gl_vertexes[vertexnum];
        if (!currentvertex)
          continue;
        // set texture coordinate of this vertex
        glTexCoord2f(currentvertex->u,currentvertex->v);
        // set vertex coordinate
        glVertex3f(currentvertex->x,0.0f,currentvertex->y);
      }
      // end of loop
      glEnd();
      if (use_display_lists)
        glEndList();
    }
    else
      glCallList(subsectorloops[flat->subsectornum].gl_list);
  }
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  // disable backside removing
  //glDisable(GL_CULL_FACE);
}

// gld_AddFlat
//
// This draws on flat for the sector "num"
// The ceiling boolean indicates if the flat is a floor(false) or a ceiling(true)

static void gld_AddFlat(int subsectornum, int sectornum, boolean ceiling, visplane_t *plane)
{
  subsector_t *subsector; // the subsector we want to draw
  sector_t *sector; // the sector we want to draw
  sector_t tempsec; // needed for R_FakeFlat
  int floorlightlevel;      // killough 3/16/98: set floor lightlevel
  int ceilinglightlevel;    // killough 4/11/98
  GLFlat flat;

  flat.subsectornum=subsectornum;
  flat.sectornum=sectornum;
  if (subsectornum>=0)
  {
    subsector=&subsectors[subsectornum]; // get the subsector
    sectornum=subsector->sector->iSectorID;
  }
  if (sectornum<0)
    return;
  sector=&sectors[sectornum]; // get the sector
  sector=R_FakeFlat(sector, &tempsec, &floorlightlevel, &ceilinglightlevel, false); // for boom effects
  flat.ceiling=ceiling;
  if (!ceiling) // if it is a floor ...
  {
    if (sector->floorpic == skyflatnum) // don't draw if sky
      return;
    // get the texture. flattranslation is maintained by doom and
    // contains the number of the current animation frame
    flat.gltexture=gld_RegisterFlat(flattranslation[sector->floorpic], true);
    if (!flat.gltexture)
      return;
    // get the lightlevel from floorlightlevel
    flat.light=gld_CalcLightLevel(floorlightlevel+(extralight<<5));
    // calculate texture offsets
    flat.uoffs=(float)sector->floor_xoffs/(float)FRACUNIT;
    flat.voffs=(float)sector->floor_yoffs/(float)FRACUNIT;
  }
  else // if it is a ceiling ...
  {
    if (sector->ceilingpic == skyflatnum) // don't draw if sky
      return;
    // get the texture. flattranslation is maintained by doom and
    // contains the number of the current animation frame
    flat.gltexture=gld_RegisterFlat(flattranslation[sector->ceilingpic], true);
    if (!flat.gltexture)
      return;
    // get the lightlevel from ceilinglightlevel
    flat.light=gld_CalcLightLevel(ceilinglightlevel+(extralight<<5));
    // calculate texture offsets
    flat.uoffs=(float)sector->ceiling_xoffs/(float)FRACUNIT;
    flat.voffs=(float)sector->ceiling_yoffs/(float)FRACUNIT;
  }
  
  // get height from plane
  flat.z=(float)plane->height/(float)MAP_SCALE;

  if (gld_drawinfo.num_flats>=gld_drawinfo.max_flats)
  {
    gld_drawinfo.max_flats+=128;
    gld_drawinfo.flats=Z_Realloc(gld_drawinfo.flats,gld_drawinfo.max_flats*sizeof(GLFlat),PU_LEVEL,0);
  }
  gld_AddDrawItem(GLDIT_FLAT, gld_drawinfo.num_flats);
  gld_drawinfo.flats[gld_drawinfo.num_flats++]=flat;
//  gld_DrawFlat(&flat);
}

void gld_AddPlane(int subsectornum, visplane_t *floorplane, visplane_t *ceilingplane)
{
  subsector_t *subsector;

  // check if all arrays are allocated
  if (!sectorrendered)
    return;
  if (!subsectorrendered)
    return;

  if (usingGLNodes)
  {
    if (subsectorrendered[subsectornum]!=rendermarker) // if not already rendered
    {
      // render the floor
      if (floorplane)
        gld_AddFlat(subsectornum, -1, false, floorplane);
      // render the ceiling
      if (ceilingplane)
        gld_AddFlat(subsectornum, -1, true, ceilingplane);
      // set rendered true
      subsectorrendered[subsectornum]=rendermarker;
    }
  }
  else
  {
    subsector = &subsectors[subsectornum];
    if (!subsector)
      return;
    if (sectorrendered[subsector->sector->iSectorID]!=rendermarker) // if not already rendered
    {
      // render the floor
      if (floorplane)
        gld_AddFlat(-1, subsector->sector->iSectorID, false, floorplane);
      // render the ceiling
      if (ceilingplane)
        gld_AddFlat(-1, subsector->sector->iSectorID, true, ceilingplane);
      // set rendered true
      sectorrendered[subsector->sector->iSectorID]=rendermarker;
    }
  }
}

/*****************
 *               *
 * Sprites       *
 *               *
 *****************/

static void gld_DrawSprite(GLSprite *sprite)
{
  gld_BindPatch(sprite->gltexture,sprite->cm);
  glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(sprite->x,sprite->y,sprite->z);
	glRotatef(inv_yaw,0.0f,1.0f,0.0f);
	if(sprite->shadow)
  {
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    //glColor4f(0.2f,0.2f,0.2f,(float)tran_filter_pct/100.0f);
    glAlphaFunc(GL_GEQUAL,0.1f);
    glColor4f(0.2f,0.2f,0.2f,0.33f);
  }
  else
  {
		if(sprite->trans)
      gld_StaticLightAlpha(sprite->light,(float)tran_filter_pct/100.0f);
		else
      gld_StaticLight(sprite->light);
  }
  glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(sprite->ul, sprite->vt);
    glVertex3f( sprite->hoff-sprite->w, sprite->voff          , 0.0f);
		glTexCoord2f(sprite->ur, sprite->vt);
    glVertex3f( sprite->hoff          , sprite->voff          , 0.0f);
		glTexCoord2f(sprite->ul, sprite->vb);
    glVertex3f( sprite->hoff-sprite->w, sprite->voff-sprite->h, 0.0f);
		glTexCoord2f(sprite->ur, sprite->vb);
    glVertex3f( sprite->hoff          , sprite->voff-sprite->h, 0.0f);
	glEnd();
		
  glPopMatrix();

	if(sprite->shadow)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL,0.5f);
  }
}

void gld_AddSprite(vissprite_t *vspr)
{
  mobj_t *pSpr=vspr->thing;
  GLSprite sprite;

  sprite.scale=vspr->scale;
	if (pSpr->frame & FF_FULLBRIGHT)
		sprite.light = 1.0f;
	else
		sprite.light = gld_CalcLightLevel(pSpr->subsector->sector->lightlevel+(extralight<<5));
  sprite.cm=CR_LIMIT+(int)((pSpr->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
  sprite.gltexture=gld_RegisterPatch(vspr->patch+firstspritelump,sprite.cm);
  if (!sprite.gltexture)
    return;
  sprite.shadow = (pSpr->flags & MF_SHADOW) != 0;
  sprite.trans  = (pSpr->flags & MF_TRANSLUCENT) != 0;
  sprite.x=-(float)pSpr->x/(float)MAP_SCALE;
  sprite.y= (float)pSpr->z/(float)MAP_SCALE;
  sprite.z= (float)pSpr->y/(float)MAP_SCALE;
  sprite.vt=0.0f;
  sprite.vb=(float)sprite.gltexture->height/(float)sprite.gltexture->tex_height;
  if (vspr->flip)
  {
    sprite.ul=0.0f;
    sprite.ur=(float)sprite.gltexture->width/(float)sprite.gltexture->tex_width;
  }
  else
  {
    sprite.ul=(float)sprite.gltexture->width/(float)sprite.gltexture->tex_width;
    sprite.ur=0.0f;
  }
  sprite.hoff=(float)sprite.gltexture->leftoffset/(float)(MAP_COEFF);
  sprite.voff=(float)sprite.gltexture->topoffset/(float)(MAP_COEFF);
  sprite.w=(float)sprite.gltexture->realtexwidth/(float)(MAP_COEFF);
  sprite.h=(float)sprite.gltexture->realtexheight/(float)(MAP_COEFF);

  if (gld_drawinfo.num_sprites>=gld_drawinfo.max_sprites)
  {
    gld_drawinfo.max_sprites+=128;
    gld_drawinfo.sprites=Z_Realloc(gld_drawinfo.sprites,gld_drawinfo.max_sprites*sizeof(GLSprite),PU_LEVEL,0);
  }
  gld_AddDrawItem(GLDIT_SPRITE, gld_drawinfo.num_sprites);
  gld_drawinfo.sprites[gld_drawinfo.num_sprites++]=sprite;
//  gld_DrawSprite(&sprite);
}

/*****************
 *               *
 * Draw          *
 *               *
 *****************/

void gld_DrawScene(player_t *player)
{
  int i,j;

  for (i=gld_drawinfo.num_drawitems; i>=0; i--)
  {
    switch (gld_drawinfo.drawitems[i].itemtype)
    {
    case GLDIT_WALL:
      for (j=(gld_drawinfo.drawitems[i].itemcount-1); j>=0; j--)
        gld_DrawWall(&gld_drawinfo.walls[j+gld_drawinfo.drawitems[i].firstitemindex]);
      break;
    case GLDIT_FLAT:
      // enable backside removing
      glEnable(GL_CULL_FACE);
      for (j=(gld_drawinfo.drawitems[i].itemcount-1); j>=0; j--)
        gld_DrawFlat(&gld_drawinfo.flats[j+gld_drawinfo.drawitems[i].firstitemindex]);
      // disable backside removing
      glDisable(GL_CULL_FACE);
      break;
    case GLDIT_SPRITE:
      for (j=(gld_drawinfo.drawitems[i].itemcount-1); j>=0; j--)
        gld_DrawSprite(&gld_drawinfo.sprites[j+gld_drawinfo.drawitems[i].firstitemindex]);
      break;
    }
  }
}

void gld_PreprocessLevel(void)
{
/*
#ifdef _DEBUG
  void gld_Precache(void);
  gld_Precache();
#endif
*/
  gld_PreprocessSectors();
  gld_PreprocessSegs();
  memset(&gld_drawinfo,0,sizeof(GLDrawInfo));
}

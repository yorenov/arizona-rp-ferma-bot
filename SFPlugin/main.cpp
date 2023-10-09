#include <Windows.h>
#include "main.h"

SAMPFUNCS *SF = new SAMPFUNCS();

bool state = false;
bool state_taked = false;
bool state_harvest = false;
float wheat_dist = 1.5;


void __stdcall mainloop()
{
	static bool initialized = false;
	if (!initialized)
	{
		if (GAME && GAME->GetSystemState() == eSystemState::GS_PLAYING_GAME && SF->getSAMP()->IsInitialized())
		{
			initialized = true;
			SF->getRakNet()->registerRakNetCallback(RAKHOOK_TYPE_INCOMING_RPC, onRecvRpc);
			SF->getSAMP()->getChat()->AddChatMessage(-1, "{CC00EE}Бот на ферму от vegas~ загружен");
			SF->getSAMP()->registerChatCommand("fermabot", [](std::string arg) {
				state = !state;
				SF->getSAMP()->getChat()->AddChatMessage(-1, state ? "{CC00EE}Бот на ферму от vegas~ {55FF00}начал работу" : "{CC00EE}Бот на ферму от vegas~ {FF0000}завершил работу");
				state_taked = false;
			});
		}
	}
	
	if (initialized) {
		if (state) {
			EngineWork();
		}
	}
}

void WalkEngine(bool eng) 
{
	state_harvest = false;

	SF->getGame()->emulateGTAKey(1, -255);
	if (eng) {
		CVector vec;
		PEDSELF->GetMoveSpeed(&vec);
		float speed = vec.Length();
		if (speed > 4 && (rand() % 150) == 1) {
			SF->getGame()->emulateGTAKey(14, 255);
		}
		else {
			SF->getGame()->emulateGTAKey(16, 255);
		}
	}
}

int GetNearWheat()
{
	float MinDist = 10000;
	float Dist;
	int nearest = -1;
	CVector* vect = PEDSELF->GetPosition();

	for (int i = 0; i < SAMP_MAX_3DTEXTS; i++)
	{
		if (SF->getSAMP()->getInfo()->pPools->pText3D->iIsListed[i] != 1)
			continue;

		std::string strdialogText(SF->getSAMP()->getInfo()->pPools->pText3D->textLabel[i].pText);
		if (strdialogText.find("Чтобы сорвать куст") != std::string::npos) {
			float labelposX = SF->getSAMP()->getInfo()->pPools->pText3D->textLabel[i].fPosition[0];
			float labelposY = SF->getSAMP()->getInfo()->pPools->pText3D->textLabel[i].fPosition[1];
			Dist = distance(vect->fX, vect->fY, labelposX, labelposY);

			if (Dist < MinDist) {
				nearest = i;
				MinDist = Dist;
			}
		}
	}
	return nearest;
}


bool EngineWork()
{
	CVector* myPos = PEDSELF->GetPosition();

	if (!state_taked) {
		int nearest = GetNearWheat();
		if (nearest == -1) {
			return false;
		}

		CVector* myPos = PEDSELF->GetPosition();
		float labelposX = SF->getSAMP()->getInfo()->pPools->pText3D->textLabel[nearest].fPosition[0];
		float labelposY = SF->getSAMP()->getInfo()->pPools->pText3D->textLabel[nearest].fPosition[1];
		int near_dist = distance(myPos->fX, myPos->fY, labelposX, labelposY);

		if (near_dist > wheat_dist) {
			cameraset(labelposX, labelposY);
			if (near_dist > wheat_dist * 3) {
				WalkEngine(true);
			}
			else 
			{
				WalkEngine(false);
			}
		}
		else if (!state_harvest) {
			if (SF->getSAMP()->getPlayers()->pLocalPlayer->sCurrentAnimID == 163) {
				state_harvest = true;
			}
			
			EmulAlt();
		}
	} 
	else
	{
		float x1 = -105.60591125488, y1 = 100.61192321777, z1 = 3.1171875;
		if (distance(x1, y1, myPos->fX, myPos->fY)) {
			cameraset(x1, y1);
			WalkEngine(true);
		}
	}
	return true;
}


void EmulAlt()
{
	stOnFootData sync;
	memset(&sync, 0, sizeof(stOnFootData));
	sync = SF->getSAMP()->getPlayers()->pLocalPlayer->onFootData;
	sync.sKeys = 1024;
	sync.stSampKeys.keys_unknown__walkSlow = 1;
	BitStream bsActorSync;
	bsActorSync.Write((BYTE)ID_PLAYER_SYNC);
	bsActorSync.Write((PCHAR)&sync, sizeof(stOnFootData));
	SF->getRakNet()->SendPacket(&bsActorSync);
	memset(&bsActorSync, 0, sizeof(BitStream));
	sync.sKeys = 0;
	bsActorSync.Write((BYTE)ID_PLAYER_SYNC);
	bsActorSync.Write((PCHAR)&sync, sizeof(stOnFootData));
	SF->getRakNet()->SendPacket(&bsActorSync);
}

void cameraset(float X, float Y) {

	CVector mypos;
	CVector enpos;
	CVector vector;
	enpos.fX = X;
	enpos.fY = Y;

	CCamera* pCamera = GAME->GetCamera();
	mypos = *pCamera->GetCam(pCamera->GetActiveCam())->GetSource();
	vector = mypos - enpos;
	float AngleX = atan2f(vector.fY, -vector.fX) - M_PI / 2;

	*(float*)0xB6F258 = -(AngleX - M_PI / 2);
}

float distance(int x1, int y1, int x2, int y2)
{
	return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) * 1.0);
}


bool __stdcall onRecvRpc(stRakNetHookParams* param) {
	if (state) {
		if (param->packetId == ScriptRPCEnumeration::RPC_ScrClientMessage) {
			UINT32 dColor;
			UINT32 dMessageLength;
			char Message[576];

			param->bitStream->ResetReadPointer();
			param->bitStream->Read(dColor);
			param->bitStream->Read(dMessageLength);
			param->bitStream->Read(Message, dMessageLength);
			std::string strdialogText(Message);
			if (strdialogText.find("Сена перетащено:") != std::string::npos && strdialogText.find("{FF6347} Теперь ваш навык фермерства") != std::string::npos) {
				state_taked = false;
			}
		}

		if (param->packetId == ScriptRPCEnumeration::RPC_ScrSetPlayerAttachedObject) {
			UINT16 playerId;
			UINT32 index;
			bool create;
			UINT32 model;

			param->bitStream->ResetReadPointer();
			param->bitStream->Read(playerId);
			param->bitStream->Read(index);
			param->bitStream->Read(create);
			param->bitStream->Read(model);

			if (!state_taked && playerId == SF->getSAMP()->getPlayers()->sLocalPlayerID && model == 2901) {
				state_taked = true;
			}
		}
	}
	return true;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
		SF->initPlugin(mainloop, hModule);
	return TRUE;
}

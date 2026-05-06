#include "TOFTSupport.h"
#include <GameData.h>
#include <xutil.h>
#include <bit>

namespace TOFTSupport {

	TESObjectCELL** pLastLoadedCell = reinterpret_cast<TESObjectCELL**>(0xED3B10);
	TESWorldSpace** pLastLoadedWorldspace = reinterpret_cast<TESWorldSpace**>(0xED3B14);
	TESTopicInfo** pLastLoadedInfo = reinterpret_cast<TESTopicInfo**>(0xED3B18);

	bool bReadingWorld = false;
	bool bReadingTOFT = false;
	bool bFinishedReadingTOFT = false;
	uint32_t uiStoredOffset = 0;

	struct NiPoint2 {
		float x, y;
	};

	struct OFFSET_DATA {
		uint32_t*	pCellFileOffsets = 0;
		NiPoint2	kOffsetMinCoords;
		NiPoint2	kOffsetMaxCoords;
		uint32_t	uiFileOffset = 0;
	};

	std::unordered_map<TESWorldSpace*, std::unordered_map<ModInfo*, OFFSET_DATA*>*> kWorldSpaceOffsets;

	OFFSET_DATA* GetOffsetData(const TESWorldSpace* apWorld, ModInfo* apFile) {
		ModInfo* pFile = ThisCall<ModInfo*>(0x4DEA80, apFile);
		{
			auto itWorld = kWorldSpaceOffsets.find(const_cast<TESWorldSpace*>(apWorld));
			if (itWorld != kWorldSpaceOffsets.end()) {
				auto& rFileMap = itWorld->second;
				auto itFile = rFileMap->find(pFile);
				if (itFile != rFileMap->end()) {
					return itFile->second;
				}
			}
		}
		
		OFFSET_DATA* pData = new OFFSET_DATA();
		memset(pData, 0, sizeof(OFFSET_DATA));
		pData->kOffsetMinCoords.x = -FLT_MAX;
		pData->kOffsetMinCoords.y = -FLT_MAX;
		pData->kOffsetMaxCoords.x = FLT_MAX;
		pData->kOffsetMaxCoords.y = FLT_MAX;

		auto itWorld = kWorldSpaceOffsets.find(const_cast<TESWorldSpace*>(apWorld));
		if (itWorld == kWorldSpaceOffsets.end()) {
			auto pFileMap = new std::unordered_map<ModInfo*, OFFSET_DATA*>();
			pFileMap->insert({ pFile, pData });
			kWorldSpaceOffsets.insert({ const_cast<TESWorldSpace*>(apWorld), pFileMap });
		}
		else {
			auto& rFileMap = itWorld->second;
			rFileMap->insert({ pFile, pData });
		}

		return pData;
	}

	int32_t GetIndexForCellCoord(const TESWorldSpace* apWorld, ModInfo* apFile, int32_t aiX, int32_t aiY) {
		OFFSET_DATA* pOffsetData = GetOffsetData(apWorld, apFile);
		if (!pOffsetData)
			return -1;

		int32_t iMinX = int32_t(pOffsetData->kOffsetMinCoords.x) >> 12;
		int32_t iMinY = int32_t(pOffsetData->kOffsetMinCoords.y) >> 12;
		int32_t iMaxX = int32_t(pOffsetData->kOffsetMaxCoords.x) >> 12;
		int32_t iMaxY = int32_t(pOffsetData->kOffsetMaxCoords.y) >> 12;
		if (aiX <= iMaxX && aiX >= iMinX && aiY <= iMaxY && aiY >= iMinY)
			return aiX + (aiY - iMinY) * (iMaxX - iMinX + 1) - iMinX;
		else
			return -1;
	}

	class Hook {
	private:
		bool SetOffset(uint32_t auiOffset) {
			return ThisCall<bool>(0x4E18C0, this, auiOffset);
		}

		uint32_t GetTESForm() {
			return ThisCall<uint32_t>(0x4E1600, this);
		}

		bool ReadChunkHeader() {
			return ThisCall<bool>(0x4E0740, this);
		}

		bool NextForm(bool abSkipIgnored) {
			return ThisCall<bool>(0x4E23F0, this, abSkipIgnored);
		}

		bool NextGroup() {
			return ThisCall<bool>(0x4E23A0, this);
		}

		void StartReadingTOFT(uint32_t uiOffsetToStore = 0, bool abSetOffset = true) {
			if (!bFinishedReadingTOFT) {
				ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
				if (!bReadingTOFT) {
					bReadingTOFT = true;
					if (uiOffsetToStore)
						uiStoredOffset = uiOffsetToStore;
					else
						uiStoredOffset = pThis->uiFileOffset;
				}
				if (abSetOffset)
					SetOffset(pThis->formInfo.formID);

				_DMESSAGE("Start of TOFT at %08X", pThis->uiFileOffset);
			}
		}

		void StopReadingTOFT(bool abIncludeForm = true) {
			if (!bFinishedReadingTOFT) {
				ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
				uint32_t uiOffset = pThis->uiFileOffset;
				uint32_t uiOldOffset = uiStoredOffset + (abIncludeForm ? sizeof(ModInfo::FormInfo) : 0);
				_DMESSAGE("End of TOFT at %08X", uiOffset);
				_DMESSAGE("Going back to %08X", uiOldOffset);
				SetOffset(uiOldOffset);
				bReadingTOFT = false;
				uiStoredOffset = 0;
			}
		}

		bool LoadCellData() {
			ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
			if (!*pLastLoadedCell) {
				_DMESSAGE("WORLDSPACE: No last loaded cell!");
				return false;
			}

			if (NextForm(true)) {
				auto& rFormData = pThis->formInfo;
				uint32_t uiForm = GetTESForm();
				if (uiForm == kFormType_TESObjectCELL) {
					// ID check
					if (pThis->formInfo.formID != (*pLastLoadedCell)->refID) {
						_DMESSAGE("WORLDSPACE: Cell ID mismatch! Expected %08X, got %08X", (*pLastLoadedCell)->refID, pThis->formInfo.formID);
						return false;
					}

					NextForm(true);

					if (!CdeclCall<bool>(0x629020, &rFormData)) {// TESObjectCELL::IsFormCellChild
						_DMESSAGE("WORLDSPACE: Cell is not child of last loaded cell!");
						return false;
					}

					uiForm = GetTESForm();
					if (uiForm != kFormType_Group || rFormData.formID != 6) {
						_DMESSAGE("WORLDSPACE: Expected GRUP of type 6 after CELL!");
						return false;
					}

					NextForm(true);

					uiForm = GetTESForm();
					if (uiForm != kFormType_Group) {
						_DMESSAGE("WORLDSPACE: Expected GRUP after CELL GRUP!");
						return false;
					}

					if (rFormData.formID == 8 && NextGroup()) {
						if (!CdeclCall<bool>(0x629020, &rFormData)) {// TESObjectCELL::IsFormCellChild
							_DMESSAGE("WORLDSPACE: Cell is not child of last loaded cell!");
							return false;
						}
					}
				}

				if (rFormData.recordType == 'PURG' && rFormData.formID == 9) {
					_DMESSAGE("WORLDSPACE: Skipping PURG 9 after cell %08X", (*pLastLoadedCell)->refID);
					ModInfo::FormInfo* pFormData = nullptr;
					if (NextForm(true))
						pFormData = &pThis->formInfo;
					else
						pFormData = nullptr;

					if (pFormData && pFormData->recordType == 'PURG' && pFormData->formID == 9) {
						if (NextForm(true))
							pFormData = &pThis->formInfo;
						else
							pFormData = nullptr;
					}

					while (pFormData) {
						_DMESSAGE("WORLDSPACE: Checking form %s", CdeclCall<const char*>(0x523860, GetTESForm()));
						uint32_t eFormType = CdeclCall<uint32_t>(0x4F8E80, rFormData.recordType); // TESForm::GetFormTypeFromFormString
						if (!CdeclCall<bool>(0x626110, eFormType)) // TESObjectCELL::IsFormTypeCellChild
							break;

						if ((rFormData.formFlags & 0x20) == 0) {
							_DMESSAGE("WORLDSPACE: Loading form %s", CdeclCall<const char*>(0x523860, GetTESForm()));
							ThisCall(0x4DB960, DataHandler::GetSingleton(), this, true, nullptr, false);
						}

						if (NextForm(true))
							pFormData = &pThis->formInfo;
						else
							pFormData = nullptr;

						if (pFormData && pFormData->recordType == 'PURG' && pFormData->formID == 9) {
							if ((*pLastLoadedCell)->refID == pFormData->formFlags) {
								if (NextForm(true))
									pFormData = &pThis->formInfo;
								else
									pFormData = nullptr;
							}
						}
					}
				}
				else {
					while (NextForm(true)) {
						uint32_t eForm = GetTESForm();
						if (eForm == kFormType_TESObjectCELL)
							break;

						if (CdeclCall<bool>(0x626110, eForm)) {
							const char* pFormName = CdeclCall<const char*>(0x523860, eForm);
							_DMESSAGE("WORLDSPACE: Loading form %s", pFormName);
							ThisCall(0x4DB960, DataHandler::GetSingleton(), this, true, nullptr, false);
						}
						else {

						}

					}

				}

				return true;
			}
			else {
				_DMESSAGE("WORLDSPACE: No next form.");
			}
		}

		void LoadWorldCells() {
			ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
			auto pIter = DataHandler::GetSingleton()->worldSpaceList.Head();
			while (pIter) {
				TESWorldSpace* pWorld = pIter->data;
				pIter = pIter->next;

				*pLastLoadedWorldspace = pWorld;

				_MESSAGE("\nLoading cells for world %s", pWorld->GetEditorID());

				OFFSET_DATA* pData = GetOffsetData(pWorld, pThis);
				const int32_t iXMin = int32_t(pData->kOffsetMinCoords.x) >> 12;
				const int32_t iYMin = int32_t(pData->kOffsetMinCoords.y) >> 12;
				const int32_t iXMax = int32_t(pData->kOffsetMaxCoords.x) >> 12;
				const int32_t iYMax = int32_t(pData->kOffsetMaxCoords.y) >> 12;
				for (int32_t aiY = iYMin; aiY <= iYMax; ++aiY) {
					for (int32_t aiX = iXMin; aiX <= iXMax; ++aiX) {
						int32_t iIndex = GetIndexForCellCoord(pWorld, pThis, aiX, aiY);
						if (iIndex >= 0 && pData->pCellFileOffsets[iIndex]) {
							_DMESSAGE("WORLDSPACE: Loading next world cell (%d, %d) at %08X", aiX, aiY, pData->pCellFileOffsets[iIndex]);
							SetOffset(pData->pCellFileOffsets[iIndex]);
							// Load cell
							ThisCall(0x4DB960, DataHandler::GetSingleton(), this, true, nullptr, false);

							LoadCellData();
						}
					}
				}
			}
		}

	public:
		uint32_t hk_GetTESForm() {
			ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
			uint32_t uiForm = GetTESForm();
			uint32_t uiOffset = pThis->uiFileOffset;
			if (uiForm == kFormType_TOFT) {
				const auto& rFormData = pThis->formInfo;
				if (rFormData.formID == 0xFFFFFFFF) {
					_DMESSAGE("Terminator TOFT at %08X", uiOffset);
				}
				else if (rFormData.formID == 0xFFFFFFFE) {
					uint32_t uiSize = rFormData.dataSize;
					_DMESSAGE("TOFT at %08X to increase the buffer size - %08X", uiOffset, uiSize);
					bFinishedReadingTOFT = true;
					return kFormType_TOFT;
				}
				else if (*pLastLoadedInfo) {
				}
				else if (*pLastLoadedWorldspace) {
					_DMESSAGE("WORLDSPACE: TOFT at %08X: %08X", uiOffset, rFormData.formID);
#if 1
					if (pThis->IsMaster()) {
						TESWorldSpace* pWorld = *pLastLoadedWorldspace;
						OFFSET_DATA* pData = GetOffsetData(pWorld, pThis);
						uint32_t uiCorrectedOffset = (rFormData.formID - uiOffset + sizeof(ModInfo::FormInfo)) + pData->uiFileOffset;
						_DMESSAGE("WORLDSPACE: Corrected offset: %08X", uiCorrectedOffset);
						if (uiCorrectedOffset && pData) {
							if (pData->pCellFileOffsets) {
								const int32_t iXMin = int32_t(pData->kOffsetMinCoords.x) >> 12;
								const int32_t iYMin = int32_t(pData->kOffsetMinCoords.y) >> 12;
								const int32_t iXMax = int32_t(pData->kOffsetMaxCoords.x) >> 12;
								const int32_t iYMax = int32_t(pData->kOffsetMaxCoords.y) >> 12;
								for (int32_t aiX = iXMin; aiX <= iXMax; ++aiX) {
									for (int32_t aiY = iYMin; aiY <= iYMax; ++aiY) {
										int32_t uiIndex = GetIndexForCellCoord(pWorld, pThis, aiX, aiY);
										if (uiIndex >= 0) {
											if (pData->pCellFileOffsets[uiIndex])
												pData->pCellFileOffsets[uiIndex] += uiCorrectedOffset;
										}
									}
								}
							}
							else {
								_DMESSAGE("WORLDSPACE: No cell offsets allocated for worldspace %08X in file %s", pWorld->refID, pThis->cFilename);
							}
						}
#endif
					}
				}
				else if (*pLastLoadedCell) {
					_DMESSAGE("CELL: Interior TOFT at %08X: %08X", uiOffset, rFormData.formID);
					StartReadingTOFT();
				}
			}
			else {
				_DMESSAGE("%s at %08X", CdeclCall<const char*>(0x523860, uiForm), uiOffset);
			}
			return uiForm;
		}

		bool hk_NextForm(bool abSkipIgnored) {
			if (bFinishedReadingTOFT) {
				LoadWorldCells();
				return false;
			}

			ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
			bool bResult = ThisCall<bool>(0x4E23F0, this, abSkipIgnored);
			if (bReadingTOFT) {
				const ModInfo::FormInfo& rFormData = pThis->formInfo;
				if (rFormData.recordType == 'GRUP' || rFormData.recordType == 'PURG') {
					StopReadingTOFT();
					return true;
				}
			}
			return bResult;
		}

		bool hk_NextChunk() {
			ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
			bool bResult = ThisCall<bool>(0x4DE560, this);
			if (!bResult && !bFinishedReadingTOFT) {
				const uint32_t uiOrgOffset = pThis->uiFileOffset;
				//_DMESSAGE("TOPICINFO: End of chunk at %08X", pThis->uiFileOffset);
				if (!bReadingTOFT) {
					if (NextForm(true)) {
						uint32_t uiForm = GetTESForm();
						if (uiForm == 0x50) { // TOFT
							uint32_t uiOffset = pThis->uiFileOffset;
							const auto& rFormData = pThis->formInfo;
							if (rFormData.formID == 0xFFFFFFFF || rFormData.formID == 0xFFFFFFFE) {
								//_DMESSAGE("TOPICINFO: Special TOFT at %08X: %08X", uiOffset, rFormData.formID);
							}
							else if (*pLastLoadedInfo) {
								//_DMESSAGE("\nTOPICINFO: TOFT at %08X: %08X", uiOffset, rFormData.formID);
								StartReadingTOFT(uiOrgOffset);
								if (NextForm(true)) {
									uint32_t uiForm = ThisCall<uint32_t>(0x4E1600, this); // TESFile::GetTESForm
									if (uiForm == kFormType_Group) {
										if (pThis->formInfo.formID == 9)
											StopReadingTOFT(false);
											return false;
									}
									return true;
								}
								StopReadingTOFT(false);
							}
						}
						else {
							SetOffset(uiOrgOffset);
						}
					}
				}
				else {
					//_DMESSAGE("TOPICINFO: End of TOFT chunk at %08X", pThis->uiFileOffset);
					StopReadingTOFT(false);
				}
			}
			return bResult;
		}

		void hk_LoadForm(ModInfo* apFile) {
			if (apFile->IsMaster()) {
				OFFSET_DATA* pData = GetOffsetData(reinterpret_cast<TESWorldSpace*>(this), apFile);
				pData->uiFileOffset = apFile->uiFileOffset;
			}
			ThisCall(0x4F7F80, this, apFile);
		}

		static uint32_t __fastcall hk_GetTESChunkEx(ModInfo* apFile, TESWorldSpace* apWorld) {
			uint32_t uiChunk = ThisCall<uint32_t>(0x4E10B0, apFile);
			if (uiChunk == 'TSFO') {
				uint32_t uiSize = apFile->uiActualChunkSize;
				if (uiSize) {
					OFFSET_DATA* pData = GetOffsetData(apWorld, apFile);
					if (pData->pCellFileOffsets)
						delete[] pData->pCellFileOffsets;

					const uint32_t uiCount = uiSize >> 2;
					
					pData->pCellFileOffsets = new uint32_t[uiCount];
					ThisCall(0x4E0470, apFile, pData->pCellFileOffsets, 0);
					if (apFile->bIsBigEndian) {
						for (uint32_t i = 0; i < uiCount; ++i) {
							pData->pCellFileOffsets[i] = std::byteswap(pData->pCellFileOffsets[i]);
						}
					}
				}
				_DMESSAGE("WORLDSPACE: OFST chunk in worldspace %08X", apWorld->refID);
			}
			else if (uiChunk == '0MAN') {
				NiPoint2 kMin;
				ThisCall(0x4E0470, apFile, &kMin, sizeof(NiPoint2));
				if (apFile->bIsBigEndian) {
					reinterpret_cast<uint32_t&>(kMin.x) = std::byteswap(reinterpret_cast<uint32_t&>(kMin.x));
					reinterpret_cast<uint32_t&>(kMin.y) = std::byteswap(reinterpret_cast<uint32_t&>(kMin.y));
				}
				if (apFile->IsMaster()) {
					OFFSET_DATA* pData = GetOffsetData(apWorld, apFile);
					pData->kOffsetMinCoords = kMin;
					_DMESSAGE("WORLDSPACE: Min Coords chunk in worldspace %08X, %f, %f", apWorld->refID, kMin.x, kMin.y);
				}
			}
			else if (uiChunk == '9MAN') {
				NiPoint2 kMax;
				ThisCall(0x4E0470, apFile, &kMax, sizeof(NiPoint2));
				if (apFile->bIsBigEndian) {
					reinterpret_cast<uint32_t&>(kMax.x) = std::byteswap(reinterpret_cast<uint32_t&>(kMax.x));
					reinterpret_cast<uint32_t&>(kMax.y) = std::byteswap(reinterpret_cast<uint32_t&>(kMax.y));
				}
				if (apFile->IsMaster()) {
					OFFSET_DATA* pData = GetOffsetData(apWorld, apFile);
					pData->kOffsetMaxCoords = kMax;
					_DMESSAGE("WORLDSPACE: Max Coords chunk in worldspace %08X, %f, %f", apWorld->refID, kMax.x, kMax.y);
				}
			}

			return uiChunk;
		}

		bool hk_FindCellInFile(ModInfo* apFile, int aiX, int aiY) {
			_DMESSAGE("WORLDSPACE: Finding cell (%d, %d) in file %s", aiX, aiY, apFile->cFilename);
			TESWorldSpace* pWorld = reinterpret_cast<TESWorldSpace*>(this);
			OFFSET_DATA* pData = GetOffsetData(pWorld, apFile);
			int32_t iIndex = GetIndexForCellCoord(pWorld, apFile, aiX, aiY);
			if (iIndex == -1 || pData->pCellFileOffsets[iIndex] == 0) {
				_DMESSAGE("WORLDSPACE: Cell (%d, %d) not found in worldspace %08X", aiX, aiY, pWorld->refID);
				return ThisCall<bool>(0x6652B0, this, apFile, aiX, aiY);
			}
			else {
				SetOffset(pData->pCellFileOffsets[iIndex]);
				_DMESSAGE("WORLDSPACE: Found cell (%d, %d) in worldspace %08X at offset %08X", aiX, aiY, pWorld->refID, pData->pCellFileOffsets[iIndex]);
				return true;
			}

		}

		struct FILE_TEX_DATA {
			uint32_t	uiTextureID;
			uint8_t		ucBlock;
			uint16_t	usIndex;
		};


		bool hk_GetChunkData(char* apData, uint32_t uiSize) {
			ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
			bool bResult = ThisCall<bool>(0x4E0470, this, apData, uiSize);
			if (bResult && pThis->bIsBigEndian) {
				FILE_TEX_DATA* pData = reinterpret_cast<FILE_TEX_DATA*>(apData);
				pData->uiTextureID = std::byteswap(pData->uiTextureID);
				pData->usIndex = std::byteswap(pData->usIndex);
			}
			return bResult;
		}

		bool hk_FindForm(TESObjectLAND* apLand) {
			ModInfo* pThis = reinterpret_cast<ModInfo*>(this);
			if (apLand->pParentCell) {
				TESObjectCELL* pCell = apLand->pParentCell;
				int32_t iX = pCell->pCellData.pCellDataExterior->iCellX;
				int32_t iY = pCell->pCellData.pCellDataExterior->iCellY;
				TESWorldSpace* pWorld = ThisCall<TESWorldSpace*>(0x627820, apLand->pParentCell);
				OFFSET_DATA* pData = GetOffsetData(pWorld, pThis);
				int32_t iIndex = GetIndexForCellCoord(pWorld, pThis, iX, iY);
				if (iIndex != -1 && pData->pCellFileOffsets && pData->pCellFileOffsets[iIndex] != 0) {
					SetOffset(pData->pCellFileOffsets[iIndex]);
					while (NextForm(true)) {
						uint32_t eForm = GetTESForm();
						if (eForm == kFormType_TESObjectCELL)
							break;

						if (eForm == kFormType_TESObjectLAND)
							return true;
					}
					return false;
				}
			}
			return false;
		}
	};

	void __declspec(naked) WorldSpaceOffsets_Asm() {
		static constexpr uint32_t uiContinueAddr = 0x667E13;
		static constexpr uint32_t uiExitAddr = 0x66810C;
		_asm {
			mov		edx, edi
			call	Hook::hk_GetTESChunkEx
			cmp     eax, ebx
			jnz		CONTINUE
			jmp		uiExitAddr
		CONTINUE:
			jmp		uiContinueAddr
		}
	}

	void InitHooks() {
		XUtil::PatchMemoryNop(0x4DE3E8, 2);
		XUtil::PatchMemoryNop(0x61D9E7, 2);
		SafeWrite8(0x4DE3FD + 2, 0xFB);
		ReplaceCallEx(0x4DCE3B, &Hook::hk_GetTESForm);
		ReplaceCallEx(0x4DCF93, &Hook::hk_NextForm);
		ReplaceCallEx(0x59FD32, &Hook::hk_NextChunk);
		ReplaceCallEx(0x667DF5, &Hook::hk_LoadForm);
		ReplaceCallEx(0x6692C0, &Hook::hk_FindCellInFile);
		ReplaceCallEx(0x61E14B, &Hook::hk_GetChunkData);
		ReplaceCallEx(0x61BCF2, &Hook::hk_FindForm);
		WriteRelJump(0x6680FF, uint32_t(WorldSpaceOffsets_Asm));
	}
}
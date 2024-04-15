#pragma once

#include <GameObjects.h>	
#include <NiObjects.h>

namespace RagdollControllerFix {
	const NiColorA White = NiColorA(1, 1, 1);
	const NiColorA Red = NiColorA(1, 0, 0);
	const NiColorA Green = NiColorA(0, 1, 0);
	const NiColorA Blue = NiColorA(0, 0, 1);
	const NiColorA Cyan = NiColorA(0, 1, 1);
	const NiColorA Magenta = NiColorA(1, 0, 1);
	const NiColorA Yellow = NiColorA(1, 1, 0);
	const NiColorA Black = NiColorA(0, 0, 0);

	static void CreateDebugNodes(bhkRagdollController* apController) {
		apController->spDebugLinesNode = StdCall<NiNode*>(0x810FA0);

		NiNode* pKeyFramedSkeleton = StdCall<NiNode*>(0x810FA0);

		NiNode* pRagdollSkeleton = StdCall<NiNode*>(0x810FA0);

		NiNode* pFinalSkeleton = StdCall<NiNode*>(0x810FA0);

		apController->spDebugLinesNode->AttachChild(pKeyFramedSkeleton, true);
		apController->spDebugLinesNode->AttachChild(pRagdollSkeleton, true);
		apController->spDebugLinesNode->AttachChild(pFinalSkeleton, true);
	}

	static void CreateBoneLines(bhkRagdollController* apController) {
		if (!apController->spDebugLinesNode || apController->spDebugLinesNode->GetChildCount() == 0)
			CreateDebugNodes(apController);

		UInt32 uiBoneCount = 0;
		UInt32 uiFinalBoneCount = apController->spRagdollShareData->pFinalSkeleton->m_parentIndices.GetSize() - 1;
		UInt32 uiRagdollBoneCount = apController->spRagdollShareData->pRagdollSkeleton->m_parentIndices.GetSize() - 1;

		NiColorA kColor = Black;
		for (UInt32 i = 0; i < apController->spDebugLinesNode->GetArrayCount(); i++) {
			NiAVObject* pChild = apController->spDebugLinesNode->GetAt(i);
			if (!pChild || !pChild->IsNiNode())
				continue;

			NiNode* pNode = (NiNode*)pChild;

			if (i == 0) {
				kColor = Red;
				uiBoneCount = uiFinalBoneCount;
			}
			else if (i == 1) {
				kColor = Blue;
				uiBoneCount = uiRagdollBoneCount;
			}
			else if (i == 2) {
				kColor = Green;
				uiBoneCount = uiFinalBoneCount;
			}

			for (UInt32 j = 0; j < uiBoneCount; j++) {
				NiPoint3 kPoints[2] = { (0,0,1), (0,0,0) };

				// BSTestObjects::MakeLine
				NiAVObject* pLine = CdeclCall<NiAVObject*>(0x528C60, &kPoints[0], &kColor, &kPoints[1], &kColor, true);
				pNode->AttachChild(pLine, true);
			}

		}
	}


	static void SetRigidBodyNamesRecurse(NiNode* node) {
		if (!node)
			return;

		auto m_collisionObject = node->m_collisionObject;
		if (m_collisionObject) {
			auto bhkNiCollisionObject = m_collisionObject->IsBhkNiCollisionObject();
			if (bhkNiCollisionObject) {
				auto worldObj = bhkNiCollisionObject->spWorldObject.data;
				if (worldObj) {
					UInt32 rigidBody = (UInt32)ThisStdCall<void*>(0x68B370, worldObj, (void*)0xF909B4); // NiDynamicCast(bhkRigidBody::ms_RTTI)
					if (rigidBody) {
						ThisStdCall(0xA4C780, (UInt32*)(*(UInt32*)(rigidBody + 8) + 0x74), node->m_pcName); // hkMeshBody::setName
					}
				}
			}
		}
		node = node->IsNiNode();
		if (!node)
			return;

		for (int i = 0; i < node->GetArrayCount(); ++i) {
			SetRigidBodyNamesRecurse((NiNode*)node->GetAt(i));
		}
	}

	static bhkRagdollController* __fastcall OnInitRagdollController(bhkRagdollController* ragdollController, void* edx, NiNode* node, int _Zero, int __Zero) {
		SetRigidBodyNamesRecurse(node);

		bhkRagdollController* pController = ThisCall<bhkRagdollController*>(0xA3F650, ragdollController, node, _Zero, __Zero);

		CreateBoneLines(pController);

		return pController;
	}

	void Init() {
		WriteRelCall(0x559A30, UInt32(OnInitRagdollController));
	}
}
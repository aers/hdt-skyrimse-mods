#include <detours.h>

#include "skse64/GameForms.h"
#include "skse64/GameReferences.h"
#include "skse64/NiObjects.h"
#include "skse64/NiGeometry.h"
#include "skse64/NiExtraData.h"

#include "Hooks.h"
#include "HookEvents.h"
#include "Offsets.h"
#include "skse64/NiNodes.h"
#include "skse64/GameRTTI.h"
#include "skse64_common/SafeWrite.h"

namespace hdt
{
	class BSFaceGenNiNodeEx : BSFaceGenNiNode
	{
		MEMBER_FN_PREFIX(BSFaceGenNiNodeEx);

	public:
		DEFINE_MEMBER_FN_HOOK(SkinAllGeometry, void, offset::BSFaceGenNiNode_SkinAllGeometry, NiNode* a_skeleton, char a_unk);
		DEFINE_MEMBER_FN_HOOK(SkinSingleGeometry, void, offset::BSFaceGenNiNode_SkinSingleGeometry, NiNode* a_skeleton, BSGeometry* a_geometry, char a_unk);

		void SkinSingleGeometry(NiNode * a_skeleton, BSGeometry* a_geometry, char a_unk)
		{
			const char* name = "";
			uint32_t formId = 0x0;
			
			if (a_skeleton->m_owner && a_skeleton->m_owner->baseForm)
			{
				auto bname = DYNAMIC_CAST(a_skeleton->m_owner->baseForm, TESForm, TESFullName);
				if (bname)
					name = bname->GetName();

				auto bnpc = DYNAMIC_CAST(a_skeleton->m_owner->baseForm, TESForm, TESNPC);

				if (bnpc && bnpc->nextTemplate)
					formId = bnpc->nextTemplate->formID;
			}
			
			_MESSAGE("SkinSingleGeometry %s %d - %s, %s, (formid %08x base form %08x head template form %08x)", a_skeleton->m_name, a_skeleton->m_children.m_size, a_geometry->m_name, name, a_skeleton->m_owner ? a_skeleton->m_owner->formID : 0x0, a_skeleton->m_owner ? a_skeleton->m_owner->baseForm->formID : 0x0, formId);

			SkinSingleHeadGeometryEvent e;
			e.skeleton = a_skeleton;
			e.geometry = a_geometry;
			e.headNode = this;
			g_skinSingleHeadGeometryEventDispatcher.dispatch(e);
			CALL_MEMBER_FN(this, SkinSingleGeometry)(a_skeleton, a_geometry, a_unk);
			e.hasSkinned = true;
			g_skinSingleHeadGeometryEventDispatcher.dispatch(e);
		}

		void SkinAllGeometry(NiNode * a_skeleton, char a_unk)
		{
			const char* name = "";
			uint32_t formId = 0x0;

			if (a_skeleton->m_owner && a_skeleton->m_owner->baseForm)
			{
				auto bname = DYNAMIC_CAST(a_skeleton->m_owner->baseForm, TESForm, TESFullName);
				if (bname)
					name = bname->GetName();

				auto bnpc = DYNAMIC_CAST(a_skeleton->m_owner->baseForm, TESForm, TESNPC);

				if (bnpc && bnpc->nextTemplate)
					formId = bnpc->nextTemplate->formID;
			}
			
			_MESSAGE("SkinAllGeometry %s %d, %s, (formid %08x base form %08x head template form %08x)", a_skeleton->m_name, a_skeleton->m_children.m_size, name, a_skeleton->m_owner ? a_skeleton->m_owner->formID : 0x0, a_skeleton->m_owner ? a_skeleton->m_owner->baseForm->formID : 0x0, formId);
			
			SkinAllHeadGeometryEvent e;
			e.skeleton = a_skeleton;
			e.headNode = this;
			g_skinAllHeadGeometryEventDispatcher.dispatch(e);
			CALL_MEMBER_FN(this, SkinAllGeometry)(a_skeleton, a_unk);
			e.hasSkinned = true;
			g_skinAllHeadGeometryEventDispatcher.dispatch(e);
		}
	};

	void hookFaceGen()
	{
		DetourAttach((void**)BSFaceGenNiNodeEx::_SkinSingleGeometry_GetPtrAddr(), (void*)GetFnAddr(&BSFaceGenNiNodeEx::SkinSingleGeometry));
		DetourAttach((void**)BSFaceGenNiNodeEx::_SkinAllGeometry_GetPtrAddr(), (void*)GetFnAddr(&BSFaceGenNiNodeEx::SkinAllGeometry));

		RelocAddr<uintptr_t> addr(offset::BSFaceGenNiNode_SkinSingleGeometry_bug);
		SafeWrite8(addr.GetUIntPtr(), 0x7);
	}

	void unhookFaceGen()
	{
		DetourDetach((void**)BSFaceGenNiNodeEx::_SkinSingleGeometry_GetPtrAddr(), (void*)GetFnAddr(&BSFaceGenNiNodeEx::SkinSingleGeometry));
		DetourDetach((void**)BSFaceGenNiNodeEx::_SkinAllGeometry_GetPtrAddr(), (void*)GetFnAddr(&BSFaceGenNiNodeEx::SkinAllGeometry));
	}
	
	struct Unk001CB0E0
	{
		MEMBER_FN_PREFIX(Unk001CB0E0);

		DEFINE_MEMBER_FN_HOOK(unk001CB0E0, NiAVObject*, offset::ArmorAttachFunction, NiNode* armor, NiNode* skeleton, void* unk3, char unk4, char unk5, void* unk6);
		NiAVObject* unk001CB0E0(NiNode* armor, NiNode* skeleton, void* unk3, char unk4, char unk5, void* unk6)
		{
			ArmorAttachEvent event;
			event.armorModel = armor;
			event.skeleton = skeleton;
			g_armorAttachEventDispatcher.dispatch(event);

			auto ret = CALL_MEMBER_FN(this, unk001CB0E0)(armor, skeleton, unk3, unk4, unk5, unk6);

			event.attachedNode = ret;
			event.hasAttached = true;
			g_armorAttachEventDispatcher.dispatch(event);

			return ret;
		}
	};

	void hookArmor()
	{
		DetourAttach((void**)Unk001CB0E0::_unk001CB0E0_GetPtrAddr(), (void*)GetFnAddr(&Unk001CB0E0::unk001CB0E0));
	}

	void unhookArmor()
	{
		DetourDetach((void**)Unk001CB0E0::_unk001CB0E0_GetPtrAddr(), (void*)GetFnAddr(&Unk001CB0E0::unk001CB0E0));
	}

	struct UnkEngine
	{
		MEMBER_FN_PREFIX(UnkEngine);

		DEFINE_MEMBER_FN_HOOK(onFrame, void, offset::GameLoopFunction);

		void onFrame();

		char unk[0x16];
		bool gamePaused; // 16
	};

	void UnkEngine::onFrame()
	{
		CALL_MEMBER_FN(this, onFrame)();
		FrameEvent e;
		e.gamePaused = this->gamePaused;
		g_frameEventDispatcher.dispatch(e);
	}

	auto oldShutdown = (void (*)(bool))(RelocationManager::s_baseAddr + offset::GameShutdownFunction);
	void shutdown(bool arg0)
	{
		g_shutdownEventDispatcher.dispatch(ShutdownEvent());
		oldShutdown(arg0);
	}

	void hookEngine()
	{
		DetourAttach((void**)UnkEngine::_onFrame_GetPtrAddr(), (void*)GetFnAddr(&UnkEngine::onFrame));
		DetourAttach((void**)&oldShutdown, (void*)shutdown);
	}

	void unhookEngine()
	{
		DetourDetach((void**)UnkEngine::_onFrame_GetPtrAddr(), (void*)GetFnAddr(&UnkEngine::onFrame));
		DetourDetach((void**)&oldShutdown, (void*)shutdown);
	}

	void hookAll()
	{
		DetourRestoreAfterWith();
		DetourTransactionBegin();
		hookEngine();
		hookArmor();
		hookFaceGen();
		DetourTransactionCommit();
	}

	void unhookAll()
	{
		DetourRestoreAfterWith();
		DetourTransactionBegin();
		unhookEngine();
		unhookArmor();
		unhookFaceGen();
		DetourTransactionCommit();
	}
}

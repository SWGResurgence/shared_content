//======================================================================
//
// SwgCuiSideToolbar.cpp
// copyright (c) 2001 Sony Online Entertainment
//
//======================================================================

#include "swgClientUserInterface/FirstSwgClientUserInterface.h"
#include "swgClientUserInterface/SwgCuiSideToolbar.h"

#include "clientGame/ClientCommandQueue.h"
#include "clientGame/ClientCommandQueueEntry.h"
#include "clientGame/ClientMacroManager.h"
#include "clientGame/CellObject.h"
#include "clientGame/ClientCommandChecks.h"
#include "clientGame/ConfigClientGame.h"
#include "clientGame/CreatureObject.h"
#include "clientGame/Game.h"
#include "clientGame/GameNetwork.h"
#include "clientGame/GroundScene.h"
#include "clientGame/GroundCombatActionManager.h"
#include "clientGame/InputScheme.h"
#include "clientGame/PlayerCreatureController.h"
#include "clientGame/PlayerObject.h"
#include "clientGame/TemplateCommandMappingManager.h"
#include "clientGame/WeaponObject.h"
#include "clientObject/ReticleManager.h"
#include "clientUserInterface/CuiAction.h"
#include "clientUserInterface/CuiActionManager.h"
#include "clientUserInterface/CuiActions.h"
#include "clientUserInterface/CuiCombatManager.h"
#include "clientUserInterface/CuiDragInfo.h"
#include "clientUserInterface/CuiDragManager.h"
#include "clientUserInterface/CuiIconManager.h"
#include "clientUserInterface/CuiInputNames.h"
#include "clientUserInterface/CuiInventoryManager.h"
#include "clientUserInterface/CuiIoWin.h"
#include "clientUserInterface/CuiManager.h"
#include "clientUserInterface/CuiMenuInfoHelper.h"
#include "clientUserInterface/CuiMessageQueueManager.h"
#include "clientUserInterface/CuiPreferences.h"
#include "clientUserInterface/CuiRadialMenuManager.h"
#include "clientUserInterface/CuiSettings.h"
#include "clientUserInterface/CuiSkillManager.h"
#include "clientUserInterface/CuiSoundManager.h"
#include "clientUserInterface/CuiSounds.h"
#include "clientUserInterface/CuiStringIdsCommand.h"
#include "clientUserInterface/CuiStringIdsSkill.h"
#include "clientUserInterface/CuiStringVariablesManager.h"
#include "clientUserInterface/CuiSystemMessageManager.h"
#include "clientUserInterface/CuiWidget3dObjectListViewer.h"
#include "clientUserInterface/CuiWorkspaceIcon.h"
#include "sharedFoundation/Clock.h"
#include "sharedFoundation/ConstCharCrcLowerString.h"
#include "sharedFoundation/Crc.h"
#include "sharedGame/CombatDataTable.h"
#include "sharedGame/Command.h"
#include "sharedGame/CommandChecks.h"
#include "sharedGame/CommandTable.h"
#include "sharedGame/GameObjectTypes.h"
#include "sharedGame/SharedObjectTemplate.h"
#include "sharedInputMap/InputMap.h"
#include "sharedInputMap/InputMap.h"
#include "sharedMessageDispatch/Message.h"
#include "sharedMessageDispatch/Transceiver.h"
#include "sharedNetworkMessages/ConsoleChannelMessages.h"
#include "sharedNetworkMessages/MessageQueueCommandTimer.h"
#include "sharedNetworkMessages/NewbieTutorialRequest.h"
#include "sharedNetworkMessages/NewbieTutorialResponse.h"
#include "sharedNetworkMessages/NewbieTutorialSetToolbarElement.h"
#include "sharedObject/NetworkIdManager.h"
#include "sharedObject/ObjectTemplate.h"
#include "sharedUtility/Callback.h"
#include "sharedUtility/CallbackReceiver.h"
#include "swgClientUserInterface/SwgCuiHud.h"
#include "swgClientUserInterface/SwgCuiHudFactory.h"
#include "swgClientUserInterface/SwgCuiHudWindowManager.h"
#include "swgClientUserInterface/SwgCuiMfdStatus.h"
#include "swgClientUserInterface/SwgCuiServerData.h"
#include "swgClientUserInterface/SwgCuiToolbar.h"

#include "UIButton.h"
#include "UIButtonStyle.h"
#include "UIColorEffector.h"
#include "UIData.h"
#include "UIDataSource.h"
#include "UIImageFragment.h"
#include "UIImage.h"
#include "UIImageStyle.h"
#include "UIManager.h"
#include "UIMessage.h"
#include "UIPage.h"
#include "UIPalette.h"
#include "UIPie.h"
#include "UIPopupMenu.h"
#include "UITabbedPane.h"
#include "UIText.h"
#include "UIUtils.h"
#include "UIVolumePage.h"
#include "UnicodeUtils.h"

#include "utf8.h"

#include <list>
#include <map>
#include <vector>


//======================================================================

namespace SwgCuiSideToolbarNamespace
{
	enum ToolbarActions
	{
		TA_toolbar_item_remove,
		TA_toolbar_item_use,
		TA_toolbar_item_info,
		TA_toolbar_action_perform,
		TA_toolbar_separator_insert,
		TA_toolbar_separator_remove,
		TA_toolbar_item_bind,
		TA_toolbar_item_describe
	};

	//@todo localize
	const char * const s_toolbarActionNames [][2] =
	{
		{ "toolbar_item_remove",      "Remove" },
		{ "toolbar_item_use",         "Use Item" },
		{ "toolbar_item_info",        "Item Info" },
		{ "toolbar_action_perform",   "Perform Action" },
		{ "toolbar_separator_insert", "Insert Separator" },
		{ "toolbar_separator_remove", "Remove Separator" },
		{ "toolbar_item_bind",        "Bind" },
		{ "toolbar_command_describe", "Describe" }
	};

	const int DEFAULT_PANE_COUNT          = 1;
	const int DEFAULT_ITEM_COUNT_PER_PANE = 36;
	const size_t COMBAT_PANE_INDEX = 0;

	const std::string cms_newbieTutorialRequestEquipToolbar ("equipToolbar");

	class KeybindingChangedCallbackReceiver : public CallbackReceiver
	{
	public:
		void performCallback ()
		{
			if (toolbar)
				toolbar->updateKeyBindings ();
		}

		SwgCuiSideToolbar * toolbar;
	};


	KeybindingChangedCallbackReceiver s_chatFontSizeCallbackReceiver[Game::ST_numTypes];

	const float SHADE_WIDGET_HEIGHT = 1.0f;
	const UIPoint SHADE_WIDGET_TOOL_OFFSET(4,4);
	const UIPoint BIG_SHADE_WIDGET_TOOL_OFFSET(3,3);

	const UILowerString SHADE_HAS_NO_COOLDOWN("nocooldown");
	const UILowerString SHADE_COMMAND_GROUP("cgroup");
	const UILowerString SHADE_COMMAND_GROUP2("cgroup2");
	const UILowerString WEAPON_TYPES_VALID("wvalid");
	const UILowerString WEAPON_TYPES_INVALID("winvalid");
	const UILowerString MIN_RANGE("min_command_range");
	const UILowerString MAX_RANGE("max_command_range");
	const UILowerString OBJECT_COMMAND_CRC("ocrc");

	const UIColor DEFAULT_TOOLBAR_COLOR(255,255,255);
	const UIColor DISABLED_TOOLBAR_COLOR(68,68,153);
	const UIColor DISABLED_TOOLBAR_OUTLINE(102,102,170);
	const UIColor TOOLBAR_ERROR_COLOR(255,0,0);

	const std::string COMMAND_DEFAULT_ACTION = "/defaultAction ";

	class DefaultCommand
	{
	public:
		DefaultCommand(const char * const command, int const toolBarIndex, bool const ground, bool const space, bool const waitForGrant);
	public:
		std::string m_command;
		int m_toolBarIndex;
		bool m_ground;
		bool m_space;
		bool m_waitForGrant;
	};

	static DefaultCommand const cs_defaultCombatItems[] =
	{
		// ground
		DefaultCommand("/rangedShot",         0, true, false, false),
		DefaultCommand("/meleeHit",           1, true, false, false),
		DefaultCommand("/bactaShot",          2, true, false, true),
		DefaultCommand("/taunt",              3, true, false, true),
		DefaultCommand("/knockdownRecovery",  8, true, false, true),
		DefaultCommand("/burstRun",           11, true, false, false),
		DefaultCommand("/covercharge",       16, true, false, true),
		DefaultCommand("/imagedesign",       17, true, false, true),
		DefaultCommand("/survey",            18, true, false, true),
		DefaultCommand("/sample",            19, true, false, true),
		DefaultCommand("/startDance",        20, true, false, true),
		DefaultCommand("/stopDance",         21, true, false, true),
		DefaultCommand("/startMusic",        22, true, false, true),
		DefaultCommand("/stopMusic",         23, true, false, true),

		// space
	};

	bool shouldAddCommand(DefaultCommand const & command);

	/************************************************************************/
	/* @brief remove and trailing _foo stuff
	/************************************************************************/
	inline void chopOff(std::string &input)
	{
		UNREF(input);
		/*std::string::size_type chop = input.find("_");

		if (chop != std::string::npos)
		{
			input.resize(chop);
		}*/
	}

	StringId s_queuedStringId("ui", "queued");
	Unicode::String s_queuedString;

	SwgCuiSideToolbar * s_activeToolbar = NULL;

	//A number from 0 to 1 that keeps the timer from being choppy when receiving server updates
	const float MIN_DIFFERENCE_TO_MOVE_TIMER_BACKWARDS = 0.25f;
	//From MIN to MAX difference, the timer only moves (difference - MIN).  Past that it moves the full amount.
	const float MAX_DIFFERENCE_TO_USE_MOVE_TIMER_TOLERANCE = 0.4f;

	uint32 const hash_primary_attack = Crc::normalizeAndCalculate("primaryAttack");

	std::string const s_objectKey("object");
	std::string const s_macroKey("macro");

	void remove();

	bool canBeSlotted(Object * const object);

	typedef std::pair<unsigned long, unsigned long> CooldownTimerMapValue;
	typedef std::map<std::pair<NetworkId, int>, CooldownTimerMapValue> CooldownTimerMap;
	CooldownTimerMap s_cooldownTimers;

	bool isRangeOkay(float minRange, float maxRange, Command const & command);
}

//======================================================================

SwgCuiSideToolbarNamespace::DefaultCommand::DefaultCommand(const char * const command, int const toolBarIndex, bool const ground, bool const space, bool const waitForGrant)
: m_command(command)
, m_toolBarIndex(toolBarIndex)
, m_ground(ground)
, m_space(space)
, m_waitForGrant(waitForGrant)
{
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbarNamespace::shouldAddCommand(DefaultCommand const & command)
{
	if (command.m_command.empty())
	{
		return false;
	}

	return command.m_ground;
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbarNamespace::canBeSlotted(Object * const object)
{
	bool result = false;

	if (object != 0)
	{
		ClientObject const * const clientObject = object->asClientObject();
		SharedObjectTemplate::GameObjectType got  = SharedObjectTemplate::GOT_none;
		if (clientObject)
			got = static_cast<SharedObjectTemplate::GameObjectType>(clientObject->getGameObjectType());

		if (got != SharedObjectTemplate::GOT_tool_survey)
		{
			Cui::MenuInfoTypes::Type const type = static_cast<Cui::MenuInfoTypes::Type>(CuiRadialMenuManager::findDefaultAction(*object));
			//WARNING(true, ("%d", type));

			switch (type)
			{
				case Cui::MenuInfoTypes::ITEM_ACTIVATE:
				case Cui::MenuInfoTypes::ITEM_DEACTIVATE:
				case Cui::MenuInfoTypes::ITEM_USE:
				case Cui::MenuInfoTypes::ITEM_USE_SELF:
				case Cui::MenuInfoTypes::ITEM_USE_OTHER:
				case Cui::MenuInfoTypes::CRAFT_START:
					result = true;
					break;
				default:
					break;
			}
		}
	}

	return result;
}

//======================================================================

using namespace SwgCuiSideToolbarNamespace;

//======================================================================

class SwgCuiSideToolbar::SwgCuiSideToolbarAction : public CuiAction
{
public:

	explicit SwgCuiSideToolbarAction (SwgCuiSideToolbar * toolbar) : CuiAction (), m_toolbar (NON_NULL (toolbar)) {}

	bool  performAction (const std::string & id, const Unicode::String &) const
	{
		NOT_NULL (m_toolbar);
		if(Game::getHudSceneType() != m_toolbar->getSceneType())
			return false;

		if      (id == CuiActions::toolbarSlot24) m_toolbar->setDefaultAction(24);
		else if (id == CuiActions::toolbarSlot25) m_toolbar->setDefaultAction(25);
		else if (id == CuiActions::toolbarSlot26) m_toolbar->setDefaultAction(26);
		else if (id == CuiActions::toolbarSlot27) m_toolbar->setDefaultAction(27);
		else if (id == CuiActions::toolbarSlot28) m_toolbar->setDefaultAction(28);
		else if (id == CuiActions::toolbarSlot29) m_toolbar->setDefaultAction(29);
		else if (id == CuiActions::toolbarSlot30) m_toolbar->setDefaultAction(30);
		else if (id == CuiActions::toolbarSlot31) m_toolbar->setDefaultAction(31);
		else if (id == CuiActions::toolbarSlot32) m_toolbar->setDefaultAction(32);
		else if (id == CuiActions::toolbarSlot33) m_toolbar->setDefaultAction(33);
		else if (id == CuiActions::toolbarSlot34) m_toolbar->setDefaultAction(34);
		else if (id == CuiActions::toolbarSlot35) m_toolbar->setDefaultAction(35);
		else
			return false;

		return true;
	}

	private:
		SwgCuiSideToolbar * m_toolbar;
		SwgCuiSideToolbarAction ();
};

//----------------------------------------------------------------------

void SwgCuiSideToolbar::install()
{
	ExitChain::add(SwgCuiSideToolbarNamespace::remove, "SwgCuiSideToolbarNamespace::remove");

	GroundCombatActionManager::registerSideSecondaryAttackCallback(SwgCuiSideToolbar::performSecondaryAttack);

}

//-----------------------------------------------------------------------------

void SwgCuiSideToolbarNamespace::remove()
{
	GroundCombatActionManager::registerSideSecondaryAttackCallback(0);

}

//----------------------------------------------------------------------

SwgCuiSideToolbar::SwgCuiSideToolbar (UIPage & page, Game::SceneType sceneType) :
SwgCuiLockableMediator         ("SwgCuiSideToolbar", page),
MessageDispatch::Receiver (),
m_toolbarPage		(0),
m_volumePage        (0),
m_volumeBackgroundPage (0),
m_action            (0),
m_draggingSlot      (-1),
m_dragCounter       (0),
m_popupPoint        (),
m_popupSlot         (-1),
m_sampleItemPage    (0),
m_callback          (new MessageDispatch::Callback),
m_iconMargin        (),
m_effectorUse       (new UIColorEffector (UIColor::white, 3.0f, false,  false, false)),
m_effectorUseChild  (new UIColorEffector (UIColor::white, 3.0f, false, false, false)),
m_sendEquipToolbar  (false),
m_volumeKeyBindings (0),
m_tabs              (0),
m_mouseDownOnSlot   (-1),
m_lastSize          (),
m_palIconFlashColor (),
m_sampleItemButton  (0),
m_playerIsSetup     (false),
m_toolbarHasLocalSettings (false),
m_sceneType(sceneType),
m_currentActionPage (0),
m_failedActionPage  (0),
m_whiteFlashPage    (0),
m_currentActionPages(),
m_failedActionPages (),
m_whiteFlashPages   (),
m_nextNextActionPage(0),
m_nextCurrentActionPage(0),
m_nextFailedActionPage(0),
m_nextWhiteFlashPage(0),
m_volumeTimersPage  (0),
m_executeMaxTimer   (0.0f),
m_executeTimer      (0.0f),
m_warmupMaxTimer    (0.0f),
m_warmupTimer       (0.0f),
m_commandExecutingCrc(0),
m_commandFailedCrc(0),
m_lastCooldownGroupReceived(-1),
m_executingGroup(-1),
m_mouseOverSlot     (-1),
m_mouseOverPage     (0),
m_clientOverrideCurrentActionCrc(0),
m_clientOverrideNextActionCommandCrc(0),
m_clientOverrideCooldownGroup(-1),
m_defaultActionSlot(-1),
m_defaultActionPage(0),
m_nextDefaultActionPage(0),
m_wrongWeaponTypePage(0),
m_wrongWeaponTypePages(),
m_nextWrongWeaponTypePage(0),
m_lastFadedHeight(0),
m_removedOldIcons(false),
m_clickedInActionBar(false),
m_sideToolbar(false)
{
	const UIData * const codeData = NON_NULL(getCodeData());
	{
		std::string dummyDoubleToolbarProp;
		if (codeData->GetPropertyNarrow(UILowerString("sideToolbar"), dummyDoubleToolbarProp))
			m_sideToolbar = true;
	}

	s_chatFontSizeCallbackReceiver[m_sceneType].toolbar = 0;

	m_effectorUse->Attach (0);
	m_effectorUseChild->Attach (0);

	//-- these objects must be part of the UI object heirarchy to have their palettes properly
	//-- reset when the user switches palettes

	getPage ().AddChild (m_effectorUse);
	getPage ().AddChild (m_effectorUseChild);

	// make room for pet faded heights
	m_lastFadedHeight = new float[DEFAULT_ITEM_COUNT_PER_PANE * 2];

	m_effectorUseChild->SetPropertyNarrow (UILowerString ("PalTarget"), "icon");

	m_toolbarItemPanes = new ToolbarItemPaneVector (DEFAULT_PANE_COUNT, DEFAULT_ITEM_COUNT_PER_PANE);

	getCodeDataObject (TUIPage, m_toolbarPage,      "SideToolBar", true);
	getCodeDataObject (TUIVolumePage, m_volumePage,            "SidevolumePage");
	getCodeDataObject (TUIVolumePage, m_volumeBackgroundPage,  "SidevolumeBackgrounds", true);
	getCodeDataObject (TUIPage, m_volumeTimersPage,      "SidevolumeTimersPage", true);
	getCodeDataObject (TUIPage,       m_sampleItemPage,        "SidesampleItemPage");
	getCodeDataObject (TUIButton,     m_sampleItemButton,      "SidesampleItemButton");
	getCodeDataObject (TUIVolumePage, m_volumeKeyBindings,     "SidevolumeKeyBindings");
	getCodeDataObject (TUITabbedPane, m_tabs,                  "sidetabs");
	getCodeDataObject (TUIPage,       m_currentActionPage,     "Sidecurrentaction", true);
	getCodeDataObject (TUIPage,       m_failedActionPage,      "Sidefailedaction", true);
	getCodeDataObject (TUIPage,       m_whiteFlashPage,        "Sidewhiteflash", true);
	getCodeDataObject (TUIPage,       m_mouseOverPage,         "Sidemouseover", true);
	getCodeDataObject (TUIPage,       m_wrongWeaponTypePage,   "Sidewrongweapontype", true);
	getCodeDataObject (TUIEffector,   m_effectorCurrent,       "SideeffectorCurrent", true);

	if(m_volumeTimersPage)
		m_volumeTimersPage->SetDoNotPackChildren(true);
	if(m_currentActionPage)
		m_currentActionPage->SetVisible(false);
	if(m_failedActionPage)
		m_failedActionPage->SetVisible(false);
	if(m_mouseOverPage)
		m_mouseOverPage->SetVisible(false);
	if(m_defaultActionPage)
		m_defaultActionPage->SetVisible(false);
	if(m_wrongWeaponTypePage)
		m_wrongWeaponTypePage->SetVisible(false);

	codeData->GetPropertyRect (UILowerString ("sideiconMargin"),   m_iconMargin);

	{
		std::string palIconFlashColor;
		if (!codeData->GetPropertyNarrow (UILowerString ("sideiconFlashColor"), palIconFlashColor))
			DEBUG_FATAL (true, ("No SwgCuiSideToolbar sideiconFlashColor"));
		m_palIconFlashColor = UILowerString (palIconFlashColor);
	}

	//-- we must attach to these samples because they can be removed from the toolbar
	m_sampleItemPage->Attach        (0);

	m_sampleItemPage->SetVisible (false);

	setupTabData ();

	connectToMessage (Game::Messages::SCENE_CHANGED);


	m_callback->connect (*this, &SwgCuiSideToolbar::onCommandRemoved,       static_cast<CreatureObject::Messages::CommandRemoved*>  (0));
	m_callback->connect (*this, &SwgCuiSideToolbar::onPlayerSetup,          static_cast<CreatureObject::Messages::PlayerSetup *>  (0));
	m_callback->connect(*this, &SwgCuiSideToolbar::onSceneChanged, static_cast<Game::Messages::SceneChanged*>(0));
	m_callback->connect(*this, &SwgCuiSideToolbar::onCurrentWeaponChanged, static_cast<CreatureObject::Messages::CurrentWeaponChanged*>(0));
	m_callback->connect(*this, &SwgCuiSideToolbar::onMaxRangeWeaponChanged,static_cast<WeaponObject::Messages::MaxRangeChanged *> (0));
	m_callback->connect(*this, &SwgCuiSideToolbar::onMinRangeWeaponChanged,static_cast<WeaponObject::Messages::MinRangeChanged *> (0));
	setupDefaults ();

	registerMediatorObject (*m_volumePage,  true);

	const UIBaseObject::UIObjectList & olist = m_volumePage->GetChildrenRef ();
	for (UIBaseObject::UIObjectList::const_iterator it = olist.begin (); it != olist.end (); ++it)
	{
		UIBaseObject * const obj = *it;
		NOT_NULL (obj);
		if (obj->IsA (TUIWidget))
			safe_cast<UIWidget *>(obj)->AddCallback (this);
	}

	s_queuedString = s_queuedStringId.localize();
	//startProcessingActions();

	if (m_toolbarPage)
	{
		setPageToLock(m_toolbarPage);
		registerMediatorObject(*m_toolbarPage, true);
	}
	else
	{
		setPageToLock(&getPage());
		registerMediatorObject(getPage(), true);
	}

	updateVisibility();

}

//----------------------------------------------------------------------

SwgCuiSideToolbar::~SwgCuiSideToolbar ()
{
	if(s_activeToolbar == this)
		s_activeToolbar = NULL;

	s_chatFontSizeCallbackReceiver[m_sceneType].toolbar = 0;

	clearWidgets ();

	m_effectorUse->Detach (0);
	m_effectorUse = 0;

	m_effectorUseChild->Detach (0);
	m_effectorUseChild = 0;

	m_callback->disconnect(*this, &SwgCuiSideToolbar::onSceneChanged, static_cast<Game::Messages::SceneChanged*>(0));
	m_callback->disconnect(*this, &SwgCuiSideToolbar::onCurrentWeaponChanged, static_cast<CreatureObject::Messages::CurrentWeaponChanged*>(0));
	m_callback->disconnect(*this, &SwgCuiSideToolbar::onMaxRangeWeaponChanged, static_cast<WeaponObject::Messages::MaxRangeChanged *> (0));
	m_callback->disconnect(*this, &SwgCuiSideToolbar::onMinRangeWeaponChanged, static_cast<WeaponObject::Messages::MinRangeChanged *> (0));
	delete m_callback;
	m_callback = 0;

	deactivate ();

	stopProcessingActions();

	m_volumePage = 0;
	m_volumeBackgroundPage = 0;
	m_sampleItemPage->Detach(0);
	m_sampleItemPage = 0;
	m_tabs = 0;

	//@todo save users's toolbar
	delete m_toolbarItemPanes;
	m_toolbarItemPanes = 0;

	m_currentActionPage = 0;
	m_failedActionPage = 0;
	m_whiteFlashPage = 0;
	m_defaultActionPage = 0;
	m_wrongWeaponTypePage = 0;

	deleteAllPagesInPool(m_currentActionPages);
	deleteAllPagesInPool(m_failedActionPages);
	deleteAllPagesInPool(m_whiteFlashPages);
	if(m_wrongWeaponTypePage)
		deleteAllPagesInPool(m_wrongWeaponTypePages);

	delete [] m_lastFadedHeight;

}

//-----------------------------------------------------------------

void SwgCuiSideToolbar::performActivate()
{

	updateKeyBindings ();

	m_lastSize = UISize::zero;

	//@todo save users toolbar and restore it here
	setIsUpdating (true);

	m_callback->connect (*this, &SwgCuiSideToolbar::onMacrosChangedReset,          static_cast<ClientMacroManager::Messages::Changed*>      (0));
	m_callback->connect (*this, &SwgCuiSideToolbar::onMacrosChangedReset,          static_cast<ClientMacroManager::Messages::Reset*>        (0));
	m_callback->connect (*this, &SwgCuiSideToolbar::onCommandTimerDataReceived,    static_cast<ClientCommandQueue::Messages::CommandTimerDataUpdated *>(0));
	m_callback->connect (*this, &SwgCuiSideToolbar::onCommandTimerDataReceived,    static_cast<PlayerCreatureController::Messages::CommandTimerDataReceived *>(0));

	s_chatFontSizeCallbackReceiver[m_sceneType].toolbar = this;
	CuiPreferences::getKeybindingsChangedCallback ().attachReceiver    (s_chatFontSizeCallbackReceiver[m_sceneType]);

	Callback * const callback = InputScheme::getResetCallback();
	if (callback != 0)
	{
		callback->attachReceiver(s_chatFontSizeCallbackReceiver[m_sceneType]);
	}

	resetAllPagesInPool(m_currentActionPages, m_nextCurrentActionPage);
	resetAllPagesInPool(m_failedActionPages, m_nextFailedActionPage);
	resetAllPagesInPool(m_whiteFlashPages, m_nextWhiteFlashPage);
	if(m_wrongWeaponTypePage)
		resetAllPagesInPool(m_wrongWeaponTypePages, m_nextWrongWeaponTypePage);

	s_activeToolbar = this;


}

//-----------------------------------------------------------------

void SwgCuiSideToolbar::performDeactivate()
{
	setIsUpdating (false);

	m_callback->disconnect (*this, &SwgCuiSideToolbar::onCommandTimerDataReceived,    static_cast<ClientCommandQueue::Messages::CommandTimerDataUpdated *>(0));
	m_callback->disconnect (*this, &SwgCuiSideToolbar::onCommandTimerDataReceived,    static_cast<PlayerCreatureController::Messages::CommandTimerDataReceived *>(0));
	m_callback->disconnect (*this, &SwgCuiSideToolbar::onMacrosChangedReset,          static_cast<ClientMacroManager::Messages::Changed*>      (0));
	m_callback->disconnect (*this, &SwgCuiSideToolbar::onMacrosChangedReset,          static_cast<ClientMacroManager::Messages::Reset*>        (0));
	CuiPreferences::getKeybindingsChangedCallback ().detachReceiver (s_chatFontSizeCallbackReceiver[m_sceneType]);

	Callback * const callback = InputScheme::getResetCallback ();
	if (callback != 0)
	{
		callback->detachReceiver(s_chatFontSizeCallbackReceiver[m_sceneType]);
	}

	s_chatFontSizeCallbackReceiver[m_sceneType].toolbar = 0;

	if(s_activeToolbar == this)
		s_activeToolbar = NULL;
}

//-----------------------------------------------------------------

void SwgCuiSideToolbar::receiveMessage(const MessageDispatch::Emitter & , const MessageDispatch::MessageBase & message)
{
	//----------------------------------------------------------------------
	if (message.isType (Game::Messages::SCENE_CHANGED))
	{
		repopulateSlots ();
		m_playerIsSetup = false;
	}
}

//----------------------------------------------------------------------

int SwgCuiSideToolbar::discoverToolbarSlot (UIWidget * context)
{
	NOT_NULL (context);

	if (!context)
		return -1;

	int count = 0;
	const UIBaseObject::UIObjectList & olist = m_volumePage->GetChildrenRef ();
	for (UIBaseObject::UIObjectList::const_iterator it = olist.begin (); it != olist.end (); ++it)
	{
		const UIBaseObject * const obj = *it;

		if (!obj->IsA (TUIWidget))
			continue;

		if (obj == context)
		{
			//Get the name and do a sscanf on it
			const UINarrowString & objName = obj->GetName();
			return atoi(objName.c_str());
		}

		++count;
	}

	WARNING (true, ("the toolbar widget wasn't on the toolbar.\n"));
	return -1;
}

//----------------------------------------------------------------------

UIWidget * SwgCuiSideToolbar::createToolbarWidget (const CuiDragInfo & item)
{
	UIWidget * widget = 0;

	static const UILowerString   iconPalProp  = UILowerString         ("PalColor");
	static const Unicode::String iconPalColor = Unicode::narrowToWide ("icon");

	if (item.type == CuiDragInfoTypes::CDIT_command)
	{
		UIImageStyle * const imageStyle = CuiIconManager::findIconImageStyle (item);

		if (imageStyle)
		{
			UIButton * const button = safe_cast<UIButton*>(m_sampleItemButton->DuplicateObject ());
			button->SetAutoPressByDrag (false);

			UIButtonStyle * const buttonStyle = CuiIconManager::findButtonStyle (item);
			if (buttonStyle)
				button->SetStyle (buttonStyle);

			button->SetIcon         (imageStyle);

			Unicode::String description;
			if (!item.str.empty())
			{
				std::string cmd(item.str);
				cmd.erase(cmd.begin());
				if (CuiSkillManager::localizeCmdDescription(Unicode::toLower(cmd), description))
				{
					button->SetTooltip(description);
				}
			}

//			button->SetProperty     (iconPalProp, iconPalColor);
			widget = button;
		}
	}

	else if (item.type == CuiDragInfoTypes::CDIT_macro)
	{
		if (ClientMacroManager::findMacroData (item.cmd))
		{
			UIImageStyle * const imageStyle = CuiIconManager::findIconImageStyle (item);

			if (imageStyle)
			{
				UIButton * const button = safe_cast<UIButton*>(m_sampleItemButton->DuplicateObject ());
				button->SetAutoPressByDrag (false);

				UIButtonStyle * const buttonStyle = CuiIconManager::findButtonStyle (item);
				if (buttonStyle)
					button->SetStyle (buttonStyle);

				button->SetIcon         (imageStyle);
//				button->SetProperty     (iconPalProp, iconPalColor);
				widget = button;
			}
		}
	}

	else if (item.type == CuiDragInfoTypes::CDIT_object)
	{
		ClientObject * const object = item.getClientObject ();

		if (object)
		{
			if (CuiInventoryManager::isOnCreature (*object))
			{
				CuiWidget3dObjectListViewer * const viewer = CuiIconManager::createObjectIcon (*object, CuiDragInfo::DragTypes::GameObject);
				NOT_NULL (viewer);

				viewer->setDrawName (false);
				viewer->setFitDistanceFactor (0.8f);
				if(viewer->getRotateSpeed() != 0.0f)
					viewer->setRotateSpeed       (0.5f);
//				viewer->SetContextToParent   (true);
				viewer->SetContextCapable    (true, true);
				viewer->SetContextCapable    (false, false);
				viewer->SetPropertyNarrow (CuiWidget3dObjectListViewer::PropertyName::TextStyleBottom, CuiIconManager::getFontPathSmall ());
				viewer->SetName ("Tool3dObj");
				widget = viewer;

				CuiIconManager::registerObjectIcon (*viewer, 0);

				widget->SetParent (UIManager::gUIManager ().GetRootPage ());

				const static UILowerString prop_palBgTint     = UILowerString ("PalBgTint");
				viewer->SetPropertyNarrow (prop_palBgTint, "back1");

				UIButtonStyle * const buttonStyle = CuiIconManager::findButtonStyle (item);
				if (buttonStyle)
				{
					buttonStyle->ApplyToWidget (*widget);
					widget->SetBackgroundOpacity (1.0f);
					widget->SetPropertyBoolean (CuiIconManager::Properties::ModifyBackgroundOpacity, false);
				}
				else
				{
					widget->SetBackgroundOpacity (0.0f);
					widget->RemoveProperty (CuiIconManager::Properties::ModifyBackgroundOpacity);
				}

				widget->Link ();
				widget->SetParent (0);
			}
		}
	}

	if (!widget)
		widget = safe_cast<UIWidget *>(m_sampleItemPage->DuplicateObject ());

	if (widget)
		item.setWidget (*widget);

	return widget;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::populateSlot (int slot)
{
	CuiDragInfo * const newItem = getToolbarItem (slot);

	if (!newItem)
		return;

	UIWidget * const parent = getToolbarItemWidget (slot);

	if (!parent)
		return;

	CuiWidget3dObjectListViewer * const viewer = dynamic_cast<CuiWidget3dObjectListViewer *>(parent);
	if (viewer)
	{
		CuiIconManager::unregisterObjectIcon (*viewer);
	}

	//-- refresh macro information
	int group = -1;
	int group2 = -1;
	bool isCommandQueueCommand = false;
	bool hasCooldownTime = false;
	uint32 weaponTypesValid = 0;
	uint32 weaponTypesInvalid = 0;
	bool foundWeaponTypeRequired = false;
	uint32 objectCommandCrc = 0;
	if (newItem->type == CuiDragInfoTypes::CDIT_macro)
	{
		const ClientMacroManager::Data * const data = ClientMacroManager::findMacroData (newItem->cmd);
		if (data)
		{
			newItem->name = Unicode::utf8ToWide (data->userDefinedName);
			newItem->str  = data->commandString;
		}
		else
			newItem->type = CuiDragInfoTypes::CDIT_none;
	}
	else if (newItem->type == CuiDragInfoTypes::CDIT_command)
	{
		if (!newItem->cmd.empty ())
		{
			const Command & cmd = CommandTable::getCommand (Crc::normalizeAndCalculate(newItem->cmd.c_str ()));

			if(!cmd.isNull())
			{
				isCommandQueueCommand = cmd.m_addToCombatQueue;
				hasCooldownTime = (cmd.m_coolTime > 0.0f || cmd.m_execTime > 0.0f || cmd.m_warmTime > 0.0f);

				group = cmd.m_coolGroup;
				weaponTypesValid = cmd.m_weaponTypesValid;
				weaponTypesInvalid = cmd.m_weaponTypesInvalid;
				foundWeaponTypeRequired = true;
			}
		}
		else if(!newItem->str.empty())
		{
			std::string newItemStr(newItem->str);
			chopOff(newItemStr);
			const Command & cmd = CommandTable::getCommand (Crc::normalizeAndCalculate(newItemStr.c_str ()));
			if(!cmd.isNull())
			{
				isCommandQueueCommand = cmd.m_addToCombatQueue;
				hasCooldownTime = (cmd.m_coolTime > 0.0f || cmd.m_coolTime2 > 0.0f || cmd.m_execTime > 0.0f || cmd.m_warmTime > 0.0f);

				group = cmd.m_coolGroup;
				group2 = cmd.m_coolGroup2;
				weaponTypesValid = cmd.m_weaponTypesValid;
				weaponTypesInvalid = cmd.m_weaponTypesInvalid;
				foundWeaponTypeRequired = true;
			}
		}
	}
	else if (newItem->type == CuiDragInfoTypes::CDIT_object)
	{
		uint32 templateCrc = getTemplateCrcForObject(*newItem);
		bool executeFromToolbar = TemplateCommandMappingManager::getExecuteFromToolbarForTemplateCrc(templateCrc);
		if(executeFromToolbar)
		{
			uint32 commandCrc = TemplateCommandMappingManager::getCommandCrcForTemplateCrc(templateCrc);
			if(commandCrc > 0)
			{
				const Command & cmd = CommandTable::getCommand (commandCrc);
				if(!cmd.isNull())
				{
					objectCommandCrc = commandCrc;
					group = cmd.m_coolGroup;
					group2 = cmd.m_coolGroup2;

					isCommandQueueCommand = cmd.m_addToCombatQueue;
					hasCooldownTime = (cmd.m_coolTime > 0.0f || cmd.m_coolTime2 > 0.0f || cmd.m_execTime > 0.0f || cmd.m_warmTime > 0.0f);
				}
			}
			else
			{
				uint32 fakeCommandGroup = TemplateCommandMappingManager::getFakeCommandGroupForTemplateCrc(templateCrc);
				if(fakeCommandGroup > 0)
				{
					group = fakeCommandGroup;
				}
			}
		}
	}

	UIWidget * content = createToolbarWidget (*newItem);

	if (!content)
	{
		content = safe_cast<UIWidget *>(m_sampleItemPage->DuplicateObject ());
		content->CopyPropertiesFrom (*parent);
	}

	content->SetVisible         (parent->IsVisible());
	content->SetEnabled         (parent->IsVisible());
	content->SetGetsInput       (parent->IsVisible());

	std::string dragAccepts;

	dragAccepts.append (CuiDragInfo::DragTypes::GameObject);
	dragAccepts.push_back (',');
	dragAccepts.append (CuiDragInfo::DragTypes::Command);
	dragAccepts.push_back (',');
	dragAccepts.append (CuiDragInfo::DragTypes::Macro);

	content->SetPropertyNarrow (UIWidget::PropertyName::DragAccepts, dragAccepts);

	content->SetDragable        (parent->IsVisible());
	content->SetAbsorbsInput    (parent->IsVisible());
	content->SetContextCapable  (parent->IsVisible(), parent->IsVisible());
	content->SetOpacityRelativeMin (1.0f);
	content->AddCallback        (this);
	const UIPoint & location = m_iconMargin.Location ();
	content->SetLocation        (location);
	content->SetSize            (parent->GetSize () - location - UIPoint (m_iconMargin.right, m_iconMargin.bottom));

	content->SetName (parent->GetName ());
	parent->SetVisible (false);

	UIVolumePage *currentPage = m_volumePage;

	if(currentPage)
	{
		currentPage->Pack ();
		currentPage->InsertChildAfter (content, parent);
		parent->RemoveCallback (this);
		currentPage->RemoveChild (parent);
		currentPage->Pack ();
	}

	content->Link ();

	UIWidget *wid = getToolbarItemWidget(slot);
	if(wid)
	{
		if(!isCommandQueueCommand && !hasCooldownTime)
		{
			wid->SetPropertyInteger (SHADE_HAS_NO_COOLDOWN, 1);
		}
		if(group != -1)
		{
			wid->SetPropertyInteger (SHADE_COMMAND_GROUP, group);
		}
		if(group2 != -1)
		{
			wid->SetPropertyInteger (SHADE_COMMAND_GROUP2, group2);
		}
		if(objectCommandCrc > 0)
		{
			wid->SetPropertyInteger (OBJECT_COMMAND_CRC, objectCommandCrc);
		}
		if(foundWeaponTypeRequired)
		{
			wid->SetPropertyInteger (WEAPON_TYPES_VALID, weaponTypesValid);
			wid->SetPropertyInteger (WEAPON_TYPES_INVALID, weaponTypesInvalid);
		}
	}

	updateCommandRange(slot);
}

//----------------------------------------------------------------------

void  SwgCuiSideToolbar::repopulateSlots ()
{
	UIString paneStr;
	UIUtils::FormatInteger(paneStr, m_tabs->GetActiveTab () + 1);

	//m_textPane->SetLocalText(paneStr);

	NOT_NULL(m_toolbarItemPanes);

	if (!m_toolbarItemPanes->empty())
	{
		for (int i = 24; i < DEFAULT_ITEM_COUNT_PER_PANE; ++i)
		{
			populateSlot(i);
			m_lastFadedHeight[i] = 0.0f;
		}
	}

	resetAllPagesInPool(m_currentActionPages, m_nextCurrentActionPage);
	resetAllPagesInPool(m_failedActionPages, m_nextFailedActionPage);

}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::setToolbarItem (int pane, int slot, const CuiDragInfo & item)
{
	if (pane < 0 || slot < 0)
		return;

	CuiDragInfo * const newItem = getToolbarItem (pane, slot);

	if (!newItem)
		return;

	*newItem = item;

	if (pane == m_tabs->GetActiveTab ())
		populateSlot (slot);
}

//----------------------------------------------------------------------

UIWidget * SwgCuiSideToolbar::getToolbarItemWidget (int slot)
{
	char numbuf [64];
	snprintf (numbuf, sizeof (numbuf), "%d", slot);

	UIVolumePage *page = m_volumePage;
	if(!page)
		return NULL;
	return static_cast<UIWidget *>(page->GetChild (numbuf));
}

//----------------------------------------------------------------------

UIWidget * SwgCuiSideToolbar::getToolbarItemBackgroundWidget (int slot)
{
	char numbuf [64];
	snprintf (numbuf, sizeof (numbuf), "%d", slot);

	UIVolumePage *page = m_volumeBackgroundPage;
	if(!page)
		return NULL;
	return static_cast<UIWidget *>(page->GetChild (numbuf));
}

//----------------------------------------------------------------------

UIWidget * SwgCuiSideToolbar::getToolbarItemShadeWidget (int slot)
{
	if(!m_volumeTimersPage)
		return NULL;
	char numbuf [64];
	snprintf (numbuf, sizeof (numbuf), "%d", slot);

	UIPage *page = m_volumeTimersPage;
	if(!page)
		return NULL;
	return static_cast<UIWidget *>(page->GetChild (numbuf));
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::clearWidgets()
{
	UIVolumePage *page = m_volumePage;
	if(!page)
		return;
	const UIBaseObject::UIObjectList & olist = page->GetChildrenRef ();
	for (UIBaseObject::UIObjectList::const_iterator vit = olist.begin (); vit != olist.end (); ++vit)
	{
		UIBaseObject * const parent = *vit;
		if (!parent->IsA (TUIWidget))
			continue;

		UIWidget * const parentWidget = safe_cast<UIWidget *>(parent);

		parentWidget->RemoveCallback (this);

		CuiWidget3dObjectListViewer * const viewer = dynamic_cast<CuiWidget3dObjectListViewer *>(parentWidget);
		if (viewer)
		{
			CuiIconManager::unregisterObjectIcon (*viewer);
		}
	}

	page->Clear ();
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbar::OnMessage(UIWidget *context, const UIMessage & msg)
{
	NOT_NULL (context);

	if (msg.Type == UIMessage::ContextRequest)
	{
		if (context->GetParent () != m_volumePage && context != m_volumePage)
			return true;

		m_popupPoint = msg.MouseCoords;
		m_popupSlot = -1;

		UIPopupMenu * const pop = new UIPopupMenu(&getPage());
		pop->SetStyle (getPage ().FindPopupStyle ());

		//-- clicking on the space between icons

		//-- clicking on an item in the toolbar

		if (context->GetParent () == m_volumePage)
		{
			m_popupSlot = discoverToolbarSlot (context);

			if (m_popupSlot < 0)
				return false;

			CuiDragInfo * const item = getToolbarItem (m_popupSlot);

			if (item)
			{
				bool needsDescribe = true;

				if (item->type == CuiDragInfoTypes::CDIT_object)
				{
					ClientObject * const obj = item->getClientObject ();

					//@ todo : radial menu default action

					if (obj)
					{
						pop->AddItem (s_toolbarActionNames [TA_toolbar_item_use][0],         s_toolbarActionNames [TA_toolbar_item_use][1]);
					}
				}

				pop->AddItem (s_toolbarActionNames [TA_toolbar_item_remove][0],      s_toolbarActionNames [TA_toolbar_item_remove][1]);

				if (needsDescribe)
					pop->AddItem (s_toolbarActionNames [TA_toolbar_item_describe][0],    s_toolbarActionNames [TA_toolbar_item_describe][1]);
			}
		}

		appendPopupOptions(pop);

		pop->SetLocation (context->GetWorldLocation () + msg.MouseCoords + UIPoint::one);
		pop->AddCallback (this);
		UIManager::gUIManager ().PushContextWidget (*pop);
		return false;
	}

	//----------------------------------------------------------------------
	//-- if we are dragging an item around within the toolbar, special handling is required.
	//-- the user can switch panes while the drag is in progress, so the original widget may
	//-- not be attached to any parents when the drag ends

	if (msg.Type == UIMessage::DragStart)
	{
		if (context->GetParent () == m_volumePage)
		{
			m_mouseDownOnSlot = -1;

			//-- when dragging toolbar items,
			//-- the dragging context is always one of the toolbar slot pages, due to the volume page event forwarding.

			const int slot = discoverToolbarSlot (context);
			DEBUG_FATAL (slot < 0, ("attempt to drag from a bogosity-afflicted slot.\n"));
			if (slot < 0)
				return false;

			m_draggingPane = m_tabs->GetActiveTab ();

			if (m_draggingPane < 0)
				return false;

			CuiDragInfo * const draggingItem = getToolbarItem (m_draggingPane, slot);
			if (!draggingItem)
				return false;

			m_draggingSlot = slot;
			m_dragCounter = UIManager::gUIManager().GetDragCounter ();

			return false;
		}
	}

	//----------------------------------------------------------------------
	//--

	if (msg.Type == UIMessage::DragEnd)
	{
		//-- we are interested in drag targets that are direct children of the volume page

		if (context->GetParent () != m_volumePage)
			return true;

		NOT_NULL (msg.DragSource);

		if (msg.DragSource == context || msg.DragSource->GetParent () == context)
			return false;

		const int slot = discoverToolbarSlot (context);
		if (slot < 0)
			return false;

		//----------------------------------------------------------------------
		//-- simply dragging around in the panel, just move widgets about

		if (m_dragCounter == UIManager::gUIManager().GetDragCounter ())
		{
			CuiDragInfo * const oldItem = getToolbarItem (m_draggingPane, m_draggingSlot);
			if (!oldItem)
				return false;

			CuiDragInfo * const destinationOldItem = getToolbarItem(m_tabs->GetActiveTab (), slot);

			if(destinationOldItem)
			{
				CuiDragInfo copyOfDestinationOldItem = *destinationOldItem;

				//-- set the new item up
				setToolbarItem (m_tabs->GetActiveTab (), slot, *oldItem);

				setToolbarItem (m_draggingPane, m_draggingSlot, copyOfDestinationOldItem);
			}
			else
			{
				setToolbarItem (m_draggingPane, m_draggingSlot, CuiDragInfo ());
			}

			if ((m_draggingSlot == m_defaultActionSlot) && (slot != m_defaultActionSlot))
			{
				setDefaultAction(slot, false);
			}

			saveSettings ();

			return false;
		}

		const CuiDragInfo droppingInfo (*msg.DragObject);

		// No dropping holocrons in the toolbar.
		if(droppingInfo.getClientObject() && ( droppingInfo.getClientObject()->getGameObjectType() == SharedObjectTemplate::GOT_chronicles_quest_holocron || droppingInfo.getClientObject()->getGameObjectType() == SharedObjectTemplate::GOT_chronicles_quest_holocron_recipe ) )
			return false;

		setToolbarItem (m_tabs->GetActiveTab (), slot, droppingInfo);

		saveSettings ();

		if (m_sendEquipToolbar)
		{
			m_sendEquipToolbar = false;

			const NewbieTutorialResponse response (cms_newbieTutorialRequestEquipToolbar);
			GameNetwork::send (response, true);
		}

		return false;
	}

	//----------------------------------------------------------------------

	if (msg.Type == UIMessage::DragOver)
	{
		context->SetDropFlagOk (false);

		if (context->GetParent () != m_volumePage)
			return true;

		NOT_NULL (msg.DragSource);

		if (msg.DragSource == context || msg.DragSource->GetParent () == context)
			return false;

		const CuiDragInfo cdinfo (*msg.DragObject);

		ClientObject * const clientObject = cdinfo.getClientObject ();

		if (!clientObject)
			context->SetDropFlagOk (true);
		else
		{
			context->SetDropFlagOk (CuiInventoryManager::isOnCreature (*clientObject));
		}

		return false;
	}

	if (msg.Type == UIMessage::LeftMouseDoubleClick)
	{

		if (context->GetParent () == m_volumePage)
		{
			const int slot = discoverToolbarSlot (context);
			if (slot < 0)
				return false;

			return false;
		}

		return true;
	}

	if (msg.Type == UIMessage::LeftMouseDown)
	{
		if (context->GetParent () == m_volumePage)
		{
			const int slot = discoverToolbarSlot (context);
			if (slot < 0)
				return true;

			m_mouseDownOnSlot = slot;
			return false;
		}

		return true;
	}

	if (msg.Type == UIMessage::RightMouseUp)
	{
		if (context == getPageToLock())
		{
			UISize size = m_volumePage->GetSize();
			UIPoint Min = m_volumePage->GetLocation() + getPageToLock()->GetWorldLocation();
			UIPoint Max = Min + size;

			UIPoint mouse = getPageToLock()->GetWorldLocation() + msg.MouseCoords;

			bool notOverVolume = !(mouse.x >= Min.x && mouse.x <= Max.x && mouse.y >= Min.y && mouse.y <= Max.y);
			if (notOverVolume)
			{
				SwgCuiLockableMediator::generateLockablePopup(context, msg);
				return false;
			}
		}
	}

	if (msg.Type == UIMessage::LeftMouseUp)
	{
		if ((context->GetParent () == m_volumePage))
		{
			const int slot = discoverToolbarSlot (context);
			if (slot < 0)
				return true;

			if (m_mouseDownOnSlot == slot)
			{
				m_clickedInActionBar = true;
				performToolbarAction(slot);
			}

			m_mouseDownOnSlot = -1;
			return false;
		}
		return true;
	}

	if (msg.Type == UIMessage::MouseEnter)
	{
		if (context->GetParent () == m_volumePage)
		{
			const int slot = discoverToolbarSlot (context);
			if (slot < 0)
				return true;

			m_mouseOverSlot = slot;
			return false;
		}
		return true;
	}

	if (msg.Type == UIMessage::MouseExit)
	{
		if (context->GetParent () == m_volumePage)
		{
			const int slot = discoverToolbarSlot (context);
			if (slot < 0)
				return true;

			m_mouseOverSlot = -1;
			return false;
		}
		return true;
	}

	return true;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::OnPopupMenuSelection (UIWidget * context)
{
	NOT_NULL (context);

	if (!context->IsA (TUIPopupMenu))
		return;

	UIPopupMenu * const pop = static_cast<UIPopupMenu *>(context);
	const UINarrowString & sel = pop->GetSelectedName ();

	//----------------------------------------------------------------------

	if (sel == s_toolbarActionNames [static_cast<size_t>(TA_toolbar_item_remove)][0])
	{
		DEBUG_FATAL (m_popupSlot < 0, ("slot out of whack for popup\n"));
		if (m_popupSlot < 0)
			return;

		const int pane = m_tabs->GetActiveTab ();

		DEBUG_FATAL (pane < 0, ("pane out of whack\n"));
		if (pane < 0)
			return;

		//-- clear out the toolbar item
		setToolbarItem(pane, m_popupSlot, CuiDragInfo ());
		saveSettings();
	}

	//----------------------------------------------------------------------

	else if (sel == s_toolbarActionNames [static_cast<size_t>(TA_toolbar_item_use)][0] ||
		sel == s_toolbarActionNames [static_cast<size_t>(TA_toolbar_action_perform)][0])
	{
		DEBUG_FATAL (m_popupSlot < 0, ("slot out of whack for popup\n"));
		if (m_popupSlot < 0)
			return;
		IGNORE_RETURN(performToolbarAction(m_popupSlot));
	}

	//----------------------------------------------------------------------

	else if (sel == s_toolbarActionNames [static_cast<size_t>(TA_toolbar_item_info)][0])
	{
		DEBUG_FATAL (m_popupSlot < 0, ("slot out of whack for popup\n"));
		if (m_popupSlot < 0)
			return;

	}

	//----------------------------------------------------------------------

	else if (sel == s_toolbarActionNames [static_cast<size_t>(TA_toolbar_separator_insert)][0])
	{

	}

	//----------------------------------------------------------------------

	else if (sel == s_toolbarActionNames [static_cast<size_t>(TA_toolbar_separator_remove)][0])
	{

	}

	//----------------------------------------------------------------------

	else if (sel == s_toolbarActionNames [static_cast<size_t>(TA_toolbar_item_bind)][0])
	{
		DEBUG_FATAL (m_popupSlot < 0, ("slot out of whack for popup\n"));
		if (m_popupSlot < 0)
			return;

	}

	//----------------------------------------------------------------------

	else if (sel == s_toolbarActionNames [TA_toolbar_item_describe][0])
	{
		DEBUG_FATAL (m_popupSlot < 0, ("slot out of whack for popup\n"));
		if (m_popupSlot < 0)
			return;

		const CuiDragInfo * const cdi = getToolbarItem (m_popupSlot);
		if (cdi)
		{
			switch (cdi->type)
			{
			case CuiDragInfoTypes::CDIT_object:
				{
					CuiActionManager::performAction (CuiActions::examine, Unicode::narrowToWide (cdi->objectId.getValueString ()));
					return;
				}
			case CuiDragInfoTypes::CDIT_command:
				{
					if (!cdi->str.empty ())
					{
						CuiActionManager::performAction (CuiActions::commandBrowserString, Unicode::narrowToWide (cdi->str));
						return;
					}
					else if (!cdi->cmd.empty ())
					{
						std::string str;
						if (CuiMessageQueueManager::findCommandString (cdi->cmd, str, false))
							CuiActionManager::performAction (CuiActions::commandBrowserString,       Unicode::narrowToWide (str));
						else
							WARNING (true, ("SwgCuiSideToolbar desctibe on bad entry"));

						return;
					}
				}
			case CuiDragInfoTypes::CDIT_macro:
				{
					CuiActionManager::performAction (CuiActions::commandBrowserMacro, Unicode::narrowToWide (cdi->cmd));
					return;
				}
			}
		}
	}
	else
	{
		SwgCuiLockableMediator::OnPopupMenuSelection(context);
	}
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::OnVolumePageSelectionChanged (UIWidget * context)
{
	UNREF (context);

}

//----------------------------------------------------------------------

int SwgCuiSideToolbar::getCurrentPane()
{
	return m_tabs->GetActiveTab();
}

//----------------------------------------------------------------------

CuiDragInfo *  SwgCuiSideToolbar::getToolbarItem (int pane, int slot)
{
	if ((slot < 24) || (slot > DEFAULT_ITEM_COUNT_PER_PANE))
	{
		return 0;
	}

	if ((pane < 0) || (pane > DEFAULT_PANE_COUNT))
	{
		return 0;
	}

	const size_t paneIndex = static_cast<size_t>(pane);

	if (paneIndex >= m_toolbarItemPanes->size ())
	{
		WARNING (true, ("the pane request (%d) for the toolbar was > data vector size (%d)", pane, static_cast<int>(m_toolbarItemPanes->size ())));
		return 0;
	}

	ToolbarItemPane & paneItems = (*m_toolbarItemPanes)[paneIndex];

	const size_t slotIndex = static_cast<size_t>(slot);

	if (slotIndex >= paneItems.size ())
	{
		WARNING (true, ("toolbar action slot requested (%d) > size of items data vector (%d)", slot, static_cast<int>(paneItems.size ())));
		return 0;
	}

	return &paneItems [slotIndex];
}
//----------------------------------------------------------------------

CuiDragInfo *  SwgCuiSideToolbar::getToolbarItem (int slot)
{
	int tab = m_tabs->GetActiveTab();

	if (tab < 0 && m_tabs->GetTabCount())
	{
		tab = 0;
		m_tabs->SetActiveTab(tab);
	}

	return getToolbarItem(tab, slot);
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::performSecondaryAttack(NetworkId const & target)
{
	UNREF(target);
	ClientCommandQueue::useCombatTargeting(true);

	// TODO: move up the call chain
	CreatureObject const * const player = Game::getPlayerCreature();
	if(player && (player->isIncapacitated() || player->isDead()))
		return;
	if (s_activeToolbar != NULL)
	{
		ClientCommandQueue::setIsSecondaryCommand(true);
		int const slot = s_activeToolbar->m_defaultActionSlot;
		IGNORE_RETURN(s_activeToolbar->performToolbarAction(slot));
		ClientCommandQueue::setIsSecondaryCommand(false);
	}

	ClientCommandQueue::useCombatTargeting(false);
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbar::performToolbarAction(int slot)
{
	long const activeTab = m_tabs->GetActiveTab();

	CuiDragInfo * const item = getToolbarItem(activeTab, slot);

	if (!item)
	{
		return false;
	}

	ClientCommandQueue::commandsAreNowFromToolbar(true);

	UIWidget * const parent = getToolbarItemWidget (slot);

	if(!parent)
		return false;

	parent->CancelEffector (*m_effectorUse);

	UIColor parentColor;
	static const UIColor grey (0x88,0x88,0x88);
	parent->SetColor (grey);
	parent->ExecuteEffector (*m_effectorUse);

	if (item->type == CuiDragInfoTypes::CDIT_none)
	{
		CuiSoundManager::play (CuiSounds::negative);
	}
	else if (!CuiDragManager::handleAction (*item))
	{
		WARNING (true, ("SwgCuiSideToolbar::performToolbarAction(%d) failed", slot));
		CuiSoundManager::play (CuiSounds::negative);
	}
	else
	{
		CuiSoundManager::play (CuiSounds::select_popup);
	}

	if((item->type == CuiDragInfoTypes::CDIT_command) || (item->type == CuiDragInfoTypes::CDIT_object))
	{
		std::string compareString;
		if(item->type == CuiDragInfoTypes::CDIT_command)
		{
			if (!item->str.empty ())
			{
				compareString = item->str;
			}
			else if (!item->cmd.empty ())
			{
				std::string str;
				if (CuiMessageQueueManager::findCommandString (item->cmd, str, false))
					compareString = str;
			}
		}
		else if(item->type == CuiDragInfoTypes::CDIT_object)
		{
			int commandCrc = 0;
			if(parent->HasProperty(OBJECT_COMMAND_CRC))
			{
				parent->GetPropertyInteger(OBJECT_COMMAND_CRC, commandCrc);
				if(commandCrc != 0)
				{
					std::string str;
					const Command & cmd = CommandTable::getCommand (commandCrc);
					if(!cmd.isNull())
					{
						compareString = "/";
						compareString += cmd.m_commandName;
					}
				}
			}
		}
		if (!compareString.empty())
		{
			if(compareString.size() >= 1)
			{
				//Note: command strings from CuiMessageQueueManager have a slash prepended, but clientcommandqueue entries do not
				const char *compareStringMinusSlash = &compareString.c_str()[1];
				const uint32 compareCrc = Crc::normalizeAndCalculate(compareStringMinusSlash);

				bool hasNextAction = false;
				uint32 sequenceId = CuiCombatManager::getPendingCommand();
				if(sequenceId)
				{
					const ClientCommandQueueEntry * const entry = ClientCommandQueue::findEntry (sequenceId);

					if (entry)
					{
						hasNextAction = true;
						m_clientOverrideNextActionCommandCrc = compareCrc;
					}
				}
								// Otherwise, if there is a pending override action, this becomes the next action
				if(!hasNextAction)
				{
					if(m_clientOverrideCurrentActionCrc)
					{
						hasNextAction = true;
						m_clientOverrideNextActionCommandCrc = compareCrc;
					}
				}
				// Otherwise, if the action you want to execute is waiting for cooldown, this becomes the next action
				if(!hasNextAction)
				{
					int group;
					if(parent->HasProperty(SHADE_COMMAND_GROUP))
					{
						parent->GetPropertyInteger(SHADE_COMMAND_GROUP, group);
						if(group != 0)
						{
							if(!isCooldownFinished(group))
							{
								hasNextAction = true;
								m_clientOverrideNextActionCommandCrc = compareCrc;
							}
						}
					}
					int group2;
					if(parent->HasProperty(SHADE_COMMAND_GROUP2))
					{
						parent->GetPropertyInteger(SHADE_COMMAND_GROUP2, group2);
						if(group2 != 0)
						{
							if(!isCooldownFinished(group2))
							{
								hasNextAction = true;
								m_clientOverrideNextActionCommandCrc = compareCrc;
							}
						}
					}
				}
				// Otherwise, this becomes the current action
				if(!hasNextAction)
				{
					int group;
					if(parent->HasProperty(SHADE_COMMAND_GROUP))
					{
						parent->GetPropertyInteger(SHADE_COMMAND_GROUP, group);
						if(group != 0)
						{
							// don't set this command as the override cooldown if it has no cooldown
							// this is to fix an issue where non-combat queue commands with no cooldown
							// were getting set as the override and never cleaned up
							int noCooldown = 0;
							if( !parent->GetPropertyInteger (SHADE_HAS_NO_COOLDOWN, noCooldown) || !noCooldown)
							{
								m_clientOverrideCooldownGroup = group;
								m_clientOverrideCurrentActionCrc = compareCrc;
							}

							setCommandExecuting(compareCrc);
						}
					}
				}
			}
		}
	}

	ClientCommandQueue::commandsAreNowFromToolbar(false);

	return true;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::onCommandRemoved (const CreatureObject::Messages::CommandRemoved::Payload & payload)
{
	const std::string & commandName = payload.second;
	const Command & cmd = CommandTable::getCommand (Crc::normalizeAndCalculate(commandName.c_str ()));

	if (cmd.isNull () || cmd.m_visibleToClients < 2)
		return;

	const CreatureObject * const player = payload.first;

	if (player != Game::getPlayerCreature ())
		return;

	const std::map<std::string, int> & cmds = player->getCommands  ();

	//-- command is still on the list
	if (cmds.find(commandName) != cmds.end ())
		return;

	const std::string & slashCommand = std::string ("/") + commandName;

	// we don't need to search the pet toolbar items, as they're only set/
	// unset manually.
	bool found = false;
	for (ToolbarItemPaneVector::iterator pit = m_toolbarItemPanes->begin (); pit != m_toolbarItemPanes->end (); ++pit)
	{
		ToolbarItemPane & items = *pit;

		for (ToolbarItemPane::iterator it = items.begin (); it != items.end (); ++it)
		{
			CuiDragInfo * item = it;
			if(!item)
				continue;

			std::string compareString;
			if (!item->str.empty ())
			{
				compareString = item->str;
			}
			else if (!item->cmd.empty ())
			{
				std::string str;
				if (CuiMessageQueueManager::findCommandString (item->cmd, str, false))
					compareString = str;
			}

			if (compareString == slashCommand)
			{
				item->clear ();
				found = true;
			}
		}
	}

	if (found)
		repopulateSlots ();
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::update (float deltaTimeSecs)
{
	CuiMediator::update (deltaTimeSecs);

	if(m_mouseOverPage)
		m_mouseOverPage->SetVisible(false);

	bool executing = true;
	m_executeTimer += deltaTimeSecs;
	if(m_executeTimer >= m_executeMaxTimer)
	{
		m_executeTimer = m_executeMaxTimer;
		executing = false;
	}

	bool warmingUp = true;
	m_warmupTimer += deltaTimeSecs;
	if(m_warmupTimer >= m_warmupMaxTimer)
	{
		m_warmupTimer = m_warmupMaxTimer;
		warmingUp = false;
	}

	bool showNewCurrentActionPages = false;
	bool showNewNextActionPages = false;
	if(!warmingUp && !executing && !m_clientOverrideCurrentActionCrc)
	{
		if(m_nextCurrentActionPage > 0)
		{
			resetAllPagesInPool(m_currentActionPages, m_nextCurrentActionPage);
		}
	}
	else if(m_nextCurrentActionPage == 0)
	{
		showNewCurrentActionPages = true;
	}

	if(m_nextNextActionPage == 0)
	{
		showNewNextActionPages = true;
	}

	// cooldown time can be extremely long, like 60 minutes, so we need to always
	// update the cooldown timers as long as the client is up and running; unfortunately,
	// ::update() only gets called when the client is actually playing the character;
	// to avoid "losing" the time when the client is not actually player the character
	// (like sitting at the character selection screen) and ::update() gets skipped,
	// we use the elapsed time since the last ::update() was called to properly
	// update the cooldown timers
	static const unsigned long unsignedLongMax = std::numeric_limits<unsigned long>::max();
	static unsigned long lastUpdateTimeMs = Clock::timeMs();

	const unsigned long currentTimeMs = Clock::timeMs();
	if (currentTimeMs != lastUpdateTimeMs)
	{
		if (s_cooldownTimers.empty())
		{
			lastUpdateTimeMs = currentTimeMs;
		}
		else
		{
			// calculate how much time has elapsed since the last
			// time ::update() was called, handling rollover
			unsigned long elapsedTimeMs = 0;

			if (lastUpdateTimeMs < currentTimeMs)
			{
				// no rollover
				elapsedTimeMs = currentTimeMs - lastUpdateTimeMs;
				lastUpdateTimeMs = currentTimeMs;
			}
			else
			{
				// rollover
				elapsedTimeMs = (unsignedLongMax - lastUpdateTimeMs) + currentTimeMs + 1;

				// make sure it really is a rollover; Clock::timeMs() uses floating
				// operation, and because of floating point inaccuracy, can cause
				// time to slightly "travel backwards" if you call Clock::timeMs()
				// too quickly consecutively, which could happen because ::update()
				// gets called each frame; when this happens the elapsedTimeMs
				// would be a huge value (billions); a normal elapsedTimeMs
				// value due do rollover would be relatively small

				// 1000000000ms = 11.57 days, meaning ::update() hasn't been called
				// for 11.57 days, meaning that the client is in an unusable state
				// anyway and probably will need to be restarted
				if (elapsedTimeMs < 1000000000)
				{
					// accept as valid rollover
					lastUpdateTimeMs = currentTimeMs;
				}
				else
				{
					// don't accept as valid rollover
					elapsedTimeMs = 0;
				}
			}

			for(CooldownTimerMap::iterator cooldowns = s_cooldownTimers.begin(); cooldowns != s_cooldownTimers.end();)
			{
				cooldowns->second.second += elapsedTimeMs;

				if (cooldowns->second.second >= cooldowns->second.first)
				{
					s_cooldownTimers.erase(cooldowns++);
				}
				else
				{
					++cooldowns;
				}
			}
		}
	}

	//WARNING(true, ("currentTimer = %f maxTimer = %f timerFactor = %f fadedHeight = %d", m_currentTimer, m_currentMaxTimer, timerFactor, fadedHeight));

	if(m_wrongWeaponTypePage)
	{
		resetAllPagesInPool(m_wrongWeaponTypePages, m_nextWrongWeaponTypePage);
	}

	if(m_volumeTimersPage)
	{
		UIBaseObject::UIObjectList olist;
		m_volumeTimersPage->GetChildren(olist);
		int slot = 24; //start at 24 because we're sideToolbar
		float fadedHeight = 0.0f;
		for (UIBaseObject::UIObjectList::iterator itObj = olist.begin (); itObj != olist.end (); ++itObj)
		{
			UIBaseObject * obj = *itObj;

			if (!obj->IsA (TUIWidget))
			{
				continue;
			}

			UIWidget *objAsWidget = static_cast<UIWidget *>(obj);

			if(!objAsWidget->IsA(TUIPie) && (objAsWidget->GetBackgroundOpacity() == 0.0f))  //seperator
			{
				continue;
			}

			UIWidget *toolWidget = getToolbarItemWidget(slot);

			if(!toolWidget)
			{
				continue;
			}

			fadedHeight = 0.0f;
			float opacity = 1.0f;

			if ((toolWidget->HasProperty(SHADE_COMMAND_GROUP)))
			{
				int group = -1;
				int group2 = -1;
				toolWidget->GetPropertyInteger(SHADE_COMMAND_GROUP, group);

				if(toolWidget->HasProperty(SHADE_COMMAND_GROUP2))
				{
					toolWidget->GetPropertyInteger(SHADE_COMMAND_GROUP2, group2);
				}

				if (group == m_clientOverrideCooldownGroup)
				{
					fadedHeight = SHADE_WIDGET_HEIGHT;
				}
				else if(warmingUp && group == m_executingGroup)
				{
					fadedHeight = SHADE_WIDGET_HEIGHT;
				}
				else
				{
					float timerFactor1 = (static_cast<float>(getCooldownProgress(group)) / 100.0f);
					float timerFactor2 = (group2 == -1) ? 0.0f : (static_cast<float>(getCooldownProgress(group2)) / 100.0f);
					float timerFactor = std::max(timerFactor1, timerFactor2);

					if (timerFactor > 0.0f)
					{
						fadedHeight = SHADE_WIDGET_HEIGHT * timerFactor;
					}
					else if (executing)
					{
						timerFactor = (m_executeMaxTimer <= 0.0f) ? 0.0f : (1.0f - (m_executeTimer / m_executeMaxTimer));
						fadedHeight = SHADE_WIDGET_HEIGHT * timerFactor;
						opacity = 0.7f;
					}
				}
			}

			if (slot < DEFAULT_ITEM_COUNT_PER_PANE)
			{
				// we need to offset the slot index into the m_lastFadedHeight array for pets
				int heightIndex = slot;
				if((m_lastFadedHeight[heightIndex] > 0.0f) && (m_lastFadedHeight[heightIndex] < fadedHeight))
				{
					float difference = (fadedHeight - m_lastFadedHeight[heightIndex]) / SHADE_WIDGET_HEIGHT;
					if(difference < MIN_DIFFERENCE_TO_MOVE_TIMER_BACKWARDS)
					{
						fadedHeight = m_lastFadedHeight[heightIndex];
					}
					else if (difference < MAX_DIFFERENCE_TO_USE_MOVE_TIMER_TOLERANCE)
					{
						fadedHeight = m_lastFadedHeight[heightIndex] + (difference - MIN_DIFFERENCE_TO_MOVE_TIMER_BACKWARDS);
					}
				}

				m_lastFadedHeight[heightIndex] = fadedHeight;
			}

			UISize oldSize = objAsWidget->GetSize();
			UIPoint toolLocation = toolWidget->GetLocation();
			UIPoint newLocation = toolLocation + SHADE_WIDGET_TOOL_OFFSET;
			objAsWidget->SetVisible(true);
			objAsWidget->SetOpacity(opacity);
			objAsWidget->SetLocation(newLocation);

			objAsWidget->SetPropertyFloat(UIPie::PropertyName::PieValue, fadedHeight);

			++slot;
		}
	}


	const int index = m_tabs->GetActiveTab ();

	if (index < 0 || index >= static_cast<int>(m_toolbarItemPanes->size ()))
		return;

	ToolbarItemPane & pane = (*m_toolbarItemPanes) [index];

	int weaponType = 999;
	CreatureObject *player = Game::getPlayerCreature();
	if(player)
	{
		WeaponObject *weapon = player->getCurrentWeapon();
		if(weapon)
		{
			weaponType = weapon->getWeaponType();
		}
	}

	int slot = 0;

	static uint32 lastActionCommandCrc = 0;
	Command const *nextActionCommand = NULL;
	uint32 nextActionCommandCrc = 0;

	if(m_clientOverrideNextActionCommandCrc)
	{
		nextActionCommandCrc = m_clientOverrideNextActionCommandCrc;
	}
	else
	{
		if(m_volumeTimersPage != NULL)
		{
			const uint32 sequenceId = CuiCombatManager::getPendingCommand();

			if(sequenceId)
			{
				const ClientCommandQueueEntry * const entry = ClientCommandQueue::findEntry (sequenceId);

				if (entry)
				{
					nextActionCommand = entry->m_command;
					nextActionCommandCrc = Crc::normalizeAndCalculate(nextActionCommand->m_commandName.c_str());
				}
			}
		}

	}

	lastActionCommandCrc = nextActionCommandCrc;

	std::set<int> populateSlotRequests;

	for (ToolbarItemPane::iterator it = pane.begin (); it != pane.end () && slot < DEFAULT_ITEM_COUNT_PER_PANE; ++it, ++slot)
	{
		UIWidget * const widget = getToolbarItemWidget (slot);

		if (widget == 0)
		{
			continue;
		}

		if(slot == m_mouseOverSlot)
		{
			if (m_mouseOverPage)
			{
				m_mouseOverPage->SetVisible(true);
				UIPoint parentPoint = m_toolbarPage ? m_toolbarPage->GetWorldLocation() : getPage().GetWorldLocation();
				UIPoint p = widget->GetWorldLocation() - parentPoint;
				m_mouseOverPage->SetLocation(p);
			}
		}

		CuiDragInfo & item = *it;

		if (item.type == CuiDragInfoTypes::CDIT_object)
		{
			CuiWidget3dObjectListViewer * const viewer = !widget->IsA (TUIPage) ? dynamic_cast<CuiWidget3dObjectListViewer *>(widget) : 0;

			if(m_volumeTimersPage)
			{
				UIWidget * const shadeWidget = getToolbarItemShadeWidget(slot);
				if (NULL != shadeWidget)
					shadeWidget->SetVisible(false);
			}

			if(viewer)
			{
				if(CuiPreferences::getRotateInventoryObjects())
					viewer->setRotateSpeed           (0.5f);
				else
					viewer->setRotateSpeed           (0.0f);
			}

			ClientObject * const clientObject = item.getClientObject ();
			if (clientObject)
			{
				const bool isOnPlayer = CuiInventoryManager::isOnCreature (*clientObject);

				if (viewer && !isOnPlayer)
				{
					item.clear ();
					populateSlotRequests.insert(slot);
				}

				if (!viewer && isOnPlayer)
				{
					populateSlotRequests.insert(slot);
				}
			}
			else
			{
				//-- clear the drag info type to none
				if (m_playerIsSetup)
					item.clear ();

				if (viewer)
					populateSlotRequests.insert(slot);
			}
		}

		if ((item.type == CuiDragInfoTypes::CDIT_command) || (item.type == CuiDragInfoTypes::CDIT_object))
		{

			if (m_volumeTimersPage)
			{
				UIWidget * const shadeWidget = getToolbarItemShadeWidget(slot);
				if (shadeWidget != 0)
				{
					shadeWidget->SetVisible(true);
				}
			}

			std::string compareString;

			uint32 compareCrc = 0;

			if (item.type == CuiDragInfoTypes::CDIT_command)
			{
				if (!item.str.empty ())
				{
					compareString = item.str;
				}
				else if (!item.cmd.empty ())
				{
					std::string str;
					if (CuiMessageQueueManager::findCommandString (item.cmd, str, false))
						compareString = str;
				}
			}
			else if (item.type == CuiDragInfoTypes::CDIT_object)
			{
				int commandCrc = 0;
				if(widget && widget->HasProperty(OBJECT_COMMAND_CRC))
				{
					widget->GetPropertyInteger(OBJECT_COMMAND_CRC, commandCrc);
					if(commandCrc != 0)
					{
						std::string str;
						const Command & cmd = CommandTable::getCommand (commandCrc);
						compareString = cmd.m_commandName;
					}
				}
			}

			if (widget && !compareString.empty())
			{
				if((widget->GetLocation().x < widget->GetParentWidget()->GetSize().x) &&
					(widget->GetLocation().y < widget->GetParentWidget()->GetSize().y))
				{
					if(compareString.size() >= 1)
					{
						//Note: command strings from CuiMessageQueueManager have a slash prepended, but clientcommandqueue entries do not

						if (item.type == CuiDragInfoTypes::CDIT_command)
						{
							compareString = compareString.substr(1);
							chopOff(compareString);
						}

						compareCrc = Crc::normalizeAndCalculate(compareString.c_str());

						if(showNewCurrentActionPages && (compareCrc == m_commandExecutingCrc))
						{
							UIPage *currentActionPage = getPageFromPool(m_currentActionPage, m_currentActionPages, m_nextCurrentActionPage);
							bool resetVisiblity = false;
							if(!currentActionPage->IsVisible())
								resetVisiblity = true;
							currentActionPage->SetVisible(true);
							UIPoint p = widget->GetWorldLocation() - getPage().GetWorldLocation();
							currentActionPage->SetLocation(p);
							if(resetVisiblity)
							{
								currentActionPage->CancelEffector(*m_effectorCurrent);
								currentActionPage->SetBackgroundColor(UIColor::white);
								currentActionPage->SetColor(UIColor::white);
							}
							currentActionPage->ExecuteEffector(*m_effectorCurrent);
						}

						if(compareCrc == m_commandFailedCrc)
						{
							if(m_failedActionPage)
							{
								UIPage *failedActionPage = getPageFromPool(m_failedActionPage, m_failedActionPages, m_nextFailedActionPage);
								failedActionPage->SetVisible(false);
								failedActionPage->SetVisible(true);
								UIPoint p = widget->GetWorldLocation() - getPage().GetWorldLocation();
								failedActionPage->SetLocation(p);
							}
							// This only needs to get shown once because it fades quickly, we don't have to worry about
							// switching panes or anything like that.
							m_commandFailedCrc = 0;
						}

						bool iconUnusable = false;
						if(widget->HasProperty(WEAPON_TYPES_VALID) && widget->IsVisible())
						{
							int weaponTypesValid = 0;
							int weaponTypesInvalid = 0;
							widget->GetPropertyInteger(WEAPON_TYPES_VALID, weaponTypesValid);
							widget->GetPropertyInteger(WEAPON_TYPES_INVALID, weaponTypesInvalid);
							if(getWeaponTypeIncompatible(weaponTypesValid, weaponTypesInvalid, weaponType))
								iconUnusable = true;
						}
						if ((widget->HasProperty(MIN_RANGE) && widget->HasProperty(MAX_RANGE)) && widget->IsVisible())
						{
							float minRange, maxRange;
							widget->GetPropertyFloat(MIN_RANGE, minRange);
							widget->GetPropertyFloat(MAX_RANGE, maxRange);

							if(!isRangeOkay(minRange, maxRange, CommandTable::getCommand(compareCrc)))
								iconUnusable = true;
						}
						if(iconUnusable && m_wrongWeaponTypePage)
						{
							UIPage *wrongWeaponTypePage = getPageFromPool(m_wrongWeaponTypePage, m_wrongWeaponTypePages, m_nextWrongWeaponTypePage);
							wrongWeaponTypePage->SetVisible(true);
							UIPoint p = widget->GetWorldLocation() - getPage().GetWorldLocation();
							wrongWeaponTypePage->SetLocation(p);
							wrongWeaponTypePage->SetAbsorbsInput(false);
						}
					}
				}
			}
		}
	}

	for (std::set<int>::const_iterator itPopulate = populateSlotRequests.begin(); itPopulate != populateSlotRequests.end(); ++itPopulate)
	{
		populateSlot(*itPopulate);
	}

	//- horizontal toolbars only
	const UISize & size = getPage ().GetSize ();

	if (size.x != m_lastSize.x)
		updateFromSizes ();

	//updatePrimaryThrottleBar();

	m_clickedInActionBar = false;
}


void SwgCuiSideToolbar::doWhiteFlash (const uint32 strCrc)
{
	if(!m_whiteFlashPage)
		return;

	resetAllPagesInPool(m_whiteFlashPages, m_nextWhiteFlashPage);
	m_whiteFlashPage->SetVisible(false);
	const int index = m_tabs->GetActiveTab ();

	if (index < 0 || index >= static_cast<int>(m_toolbarItemPanes->size ()))
		return;
	if (strCrc != 0)
	{
		ToolbarItemPane & pane = (*m_toolbarItemPanes) [index];

		int slot = 0;
		for (ToolbarItemPane::iterator it = pane.begin (); it != pane.end (); ++it, ++slot)
		{
			CuiDragInfo & item = *it;
			if (item.type == CuiDragInfoTypes::CDIT_command || item.type == CuiDragInfoTypes::CDIT_object)
			{
				std::string compareString;
				if(item.type == CuiDragInfoTypes::CDIT_command)
				{
					if (!item.str.empty ())
					{
						compareString = item.str;
					}
					else if (!item.cmd.empty ())
					{
						std::string str;
						if (CuiMessageQueueManager::findCommandString (item.cmd, str, false))
							compareString = str;
					}
				}
				else if (item.type == CuiDragInfoTypes::CDIT_object)
				{
					UIWidget * const widget = getToolbarItemWidget (slot);
					int commandCrc = 0;
					if(widget != 0 && widget->HasProperty(OBJECT_COMMAND_CRC))
					{
						widget->GetPropertyInteger(OBJECT_COMMAND_CRC, commandCrc);
						if(commandCrc != 0)
						{
							std::string str;
							const Command & cmd = CommandTable::getCommand (commandCrc);
							compareString = "/";
							compareString += cmd.m_commandName;
						}
					}
				}
				if(compareString.size() >= 1)
				{
					const char *compareStringMinusSlash = &compareString.c_str()[1];
					uint32 compareCrc = Crc::normalizeAndCalculate(compareStringMinusSlash);

					if(compareCrc == strCrc)
					{
						UIWidget * const widget = getToolbarItemWidget (slot);
						UIPage *whiteFlashPage = getPageFromPool(m_whiteFlashPage, m_whiteFlashPages, m_nextWhiteFlashPage);
						if (widget != 0 && whiteFlashPage != 0)
						{
							UIPoint p = widget->GetWorldLocation() - getPage().GetWorldLocation();
							whiteFlashPage->SetLocation(p);
						}
					}
				}
			}
		}
	}
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::updateFromSizes ()
{
	const UISize & size    = getPage ().GetSize ();
	const UISize & minSize = getPage ().GetMinimumSize ();

	const bool dividersShouldBeVisible    = (size.x > minSize.x);

	{
		m_volumePage->SetPackDirty(true);
		m_volumePage->Pack ();
		m_volumePage->ForcePackChildren ();

		if(m_volumeBackgroundPage)
		{
			m_volumeBackgroundPage->SetPackDirty(true);
			m_volumeBackgroundPage->Pack();
			m_volumeBackgroundPage->ForcePackChildren ();
		}
	}

	{
		const UIBaseObject::UIObjectList & olist = m_volumeKeyBindings->GetChildrenRef ();

		for (UIBaseObject::UIObjectList::const_iterator it = olist.begin (); it != olist.end (); ++it)
		{
			UIBaseObject * const obj = *it;
			if (!obj->IsA (TUIWidget))
				continue;

			UIWidget * const wid = safe_cast<UIWidget *>(*it);

			if (wid->IsA (TUIText))
				continue;

			wid->SetVisible (dividersShouldBeVisible);
		}

		m_volumeKeyBindings->Pack ();
	}
	m_lastSize = size;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::setupDefaults()
{
	repopulateSlots ();
}

namespace
{
	namespace Settings
	{
		const std::string numPanes = "sidenumPanes";
		const std::string numItems = "sidenumItems";
		const std::string itemCmd  = "sideitemCmd";
		const std::string itemStr  = "sideitemStr";
		const std::string itemType = "sideitemType";
		const std::string itemId   = "sideitemId";
		const std::string defaultActionSlot = "sidedefaultActionSlot";
		const std::string removedOldIcons = "sideremovedOldIcons";
		const std::string toolbarLocationX = "sidetoolbarLocationX";
		const std::string toolbarLocationY = "sidetoolbarLocationY";
	}
}
//----------------------------------------------------------------------

void SwgCuiSideToolbar::saveSettings         () const
{
	SwgCuiLockableMediator::saveSettings ();

	CuiSettings::saveBoolean (getMediatorDebugName (), Settings::removedOldIcons, m_removedOldIcons);

	const int numPanes = static_cast<int>(m_toolbarItemPanes->size ());
	CuiSettings::saveInteger (getMediatorDebugName (), Settings::numPanes, numPanes);

	if (m_toolbarPage)
	{
		CuiSettings::saveInteger(getMediatorDebugName(), Settings::toolbarLocationX, m_toolbarPage->GetLocation().x);
		CuiSettings::saveInteger(getMediatorDebugName(), Settings::toolbarLocationY, m_toolbarPage->GetLocation().y);
	}

	int firstNumItems = 0;

	int pane = 0;
	for (ToolbarItemPaneVector::const_iterator it = m_toolbarItemPanes->begin (); it != m_toolbarItemPanes->end (); ++it, ++pane)
	{
		const ToolbarItemPane & items = *it;

		const int numItems = static_cast<int>(items.size ());

		if (pane == 0)
			firstNumItems = numItems;

		if (numItems != firstNumItems)
		{
			WARNING (true, ("SwgCuiSideToolbar error saving toolbar.  Pane [%d] has [%d] items, but first panes had [%d] items.", pane, numItems, firstNumItems));
			break;
		}

		char buf [128];
		snprintf (buf, sizeof (buf), "%s_%d", Settings::numItems.c_str (), pane);
		CuiSettings::saveInteger (getMediatorDebugName (), buf, numItems);

		int itemIndex = 0;
		for (ToolbarItemPane::const_iterator iit = items.begin (); iit != items.end (); ++iit, ++itemIndex)
		{
			const CuiDragInfo & item = *iit;

			const char * const dragTypeString = CuiDragInfo::getDragTypeString (item.type);

			if (dragTypeString)
			{
				snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemType.c_str (), pane, itemIndex);
				CuiSettings::saveData (getMediatorDebugName (), buf, dragTypeString);

				if (!item.cmd.empty ())
				{
					snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemCmd.c_str (), pane, itemIndex);
					CuiSettings::saveData (getMediatorDebugName (), buf, item.cmd);
				}

				if (!item.str.empty ())
				{
					snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemStr.c_str (), pane, itemIndex);
					CuiSettings::saveData (getMediatorDebugName (), buf, item.str);
				}

				if (item.objectId.isValid ())
				{
					snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemId.c_str (), pane, itemIndex);
					CuiSettings::saveData (getMediatorDebugName (), buf, item.objectId.getValueString ());
				}
			}
			else
			{
				static const std::string emptyString;
				//-- erase the entry
				snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemType.c_str (), pane, itemIndex);
				CuiSettings::saveData (getMediatorDebugName (), buf, emptyString);
				snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemCmd.c_str (), pane, itemIndex);
				CuiSettings::saveData (getMediatorDebugName (), buf, emptyString);
				snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemStr.c_str (), pane, itemIndex);
				CuiSettings::saveData (getMediatorDebugName (), buf, emptyString);
				snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemId.c_str (), pane, itemIndex);
				CuiSettings::saveData (getMediatorDebugName (), buf, emptyString);
			}
		}
	}

	WARNING (pane != numPanes, ("SwgCuiSideToolbar could not save all the toolbar panes, loaded %d of %d", pane, numPanes));
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::loadSettings         ()
{
	setSettingsAutoSizeLocation(false, false);
	SwgCuiLockableMediator::loadSettings ();


	int numPanes = 0;

	if (!CuiSettings::loadInteger (getMediatorDebugName (), Settings::numPanes, numPanes))
	{
		//set auto attack to rangedShot
		setDefaultAction(0, false);
		return;
	}

	{
		int locationX;
		int locationY;
		if (CuiSettings::loadInteger(getMediatorDebugName(), Settings::toolbarLocationX, locationX) &&
			CuiSettings::loadInteger(getMediatorDebugName(), Settings::toolbarLocationY, locationY))
		{
			if (m_toolbarPage)
			{
				m_toolbarPage->SetLocation(locationX, locationY, false);
			}
		}
	}

	m_toolbarHasLocalSettings = true;

	m_toolbarItemPanes->clear  ();
	m_toolbarItemPanes->resize (numPanes);

	setupTabData ();

	int firstNumItems = 0;

	int pane = 0;
	for (ToolbarItemPaneVector::iterator it = m_toolbarItemPanes->begin (); it != m_toolbarItemPanes->end (); ++it, ++pane)
	{
		ToolbarItemPane & items = *it;

		char buf [128];
		snprintf (buf, sizeof (buf), "%s_%d", Settings::numItems.c_str (), pane);

		int numItems = 0;
		if (!CuiSettings::loadInteger (getMediatorDebugName (), buf, numItems))
		{
			WARNING (true, ("SwgCuiSideToolbar could not load num items for pane %d", pane));
			break;
		}

		if (pane == 0)
			firstNumItems = numItems;

		if (numItems != firstNumItems)
		{
			WARNING (true, ("SwgCuiSideToolbar error loading toolbar.  Pane [%d] has [%d] items, but first panes had [%d] items.", pane, numItems, firstNumItems));
			break;
		}

		items.resize (DEFAULT_ITEM_COUNT_PER_PANE);

		int itemIndex = 0;
		for (ToolbarItemPane::iterator iit = items.begin (); iit != items.end (); ++iit, ++itemIndex)
		{
			CuiDragInfo & item = *iit;

			item.type = CuiDragInfoTypes::CDIT_none;

			std::string dragTypeString;
			snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemType.c_str (), pane, itemIndex);
			CuiSettings::loadData (getMediatorDebugName (), buf, dragTypeString);

			if (!dragTypeString.empty ())
			{
				item.type = CuiDragInfo::getDragType (dragTypeString);

				if (item.type != CuiDragInfoTypes::CDIT_none)
				{
					snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemCmd.c_str (), pane, itemIndex);
					CuiSettings::loadData (getMediatorDebugName (), buf, item.cmd);

					snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemStr.c_str (), pane, itemIndex);
					CuiSettings::loadData (getMediatorDebugName (), buf, item.str);

					std::string idStr;
					snprintf (buf, sizeof (buf), "%s_%d_%d", Settings::itemId.c_str (), pane, itemIndex);
					if (CuiSettings::loadData (getMediatorDebugName (), buf, idStr))
						item.objectId = NetworkId (idStr);
					else if (item.type == CuiDragInfoTypes::CDIT_object)
						WARNING (true, ("SwgCuiSideToolbar loaded type object but could not find NetworkId for toolbar pane %d, item %d", pane, itemIndex));
				}
				else
					WARNING (true, ("SwgCuiSideToolbar loaded invalid drag type item [%s] for toolbar pane %d, item %d", dragTypeString.c_str (), pane, itemIndex));
			}
		}

		WARNING (itemIndex != numItems, ("SwgCuiSideToolbar could not load all the items for toolbar pane %d, loaded %d of %d", pane, itemIndex, numItems));

	}
	WARNING (pane != numPanes, ("SwgCuiSideToolbar could not load all the toolbar panes, loaded %d of %d", pane, numPanes));

	repopulateSlots ();

	int defaultActionSlot = 0;
	//CuiSettings::loadInteger (getMediatorDebugName (), Settings::defaultActionSlot, defaultActionSlot);
	setDefaultAction(defaultActionSlot, false);

	CuiSettings::loadBoolean (getMediatorDebugName (), Settings::removedOldIcons, m_removedOldIcons);

	removeOldStyleIcons();
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::updateKeyBindings ()
{
	const UIBaseObject::UIObjectList & olist = m_volumeKeyBindings->GetChildrenRef ();

	const InputMap * const inputMap = Game::getGameInputMap ();
	if (!inputMap)
	{
		WARNING (true, ("No inputmap"));
		return;
	}

	Unicode::String bindStr;
	char cmdname [64];
	const size_t cmdname_size = sizeof (cmdname);

	int index = 24;

	for (UIBaseObject::UIObjectList::const_iterator it = olist.begin (); it != olist.end (); ++it)
	{
		UIBaseObject * const obj = *it;
		if (obj->IsA (TUIText))
		{
			UIText * const text = safe_cast<UIText *>(obj);

			snprintf (cmdname, cmdname_size, "CMD_uiToolbarSlot%02d", index);
			bindStr.clear ();

			if (!CuiInputNames::getInputValueString (*inputMap, cmdname, bindStr))
				WARNING (true, ("no binding string for [%s]", cmdname));
			else
			{
				text->SetPreLocalized (true);
				text->SetLocalText    (bindStr);
			}

			++index;
		}
	}
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::setupTabData ()
{
	{
		m_tabs->Clear ();

		const int numPanes = static_cast<int>(m_toolbarItemPanes->size ());
		UIString str;
		for (int i = 0; i < numPanes; ++i)
		{
			str.clear ();
			UIUtils::FormatInteger (str, i);
			IGNORE_RETURN(m_tabs->AppendTab(str, 0));
		}
	}
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::onMacrosChangedReset         (const ClientMacroManager::Messages::Changed::Payload &)
{
	repopulateSlots  ();
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::onPlayerSetup (CreatureObject::Messages::PlayerSetup::Payload const & creature)
{
	for(int slot = 24; slot < DEFAULT_ITEM_COUNT_PER_PANE; ++slot)
	{
		updateCommandRange(slot);
	}

	if (!Game::getSinglePlayer ())
	{
		m_playerIsSetup = true;
	}

	PlayerObject const * const player = creature.getPlayerObject();

	if (!player || (player != Game::getPlayerObject()))
	{
		return;
	}

	if (m_toolbarHasLocalSettings)
	{
		return;
	}

	m_toolbarHasLocalSettings = true;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::onCommandTimerDataReceived   (const MessageQueueCommandTimer & commandTimerData)
{
	const uint32 commandNameCrc = commandTimerData.getCommandNameCrc();

	if (commandNameCrc == hash_primary_attack)
	{
		return;
	}

#if 0
	uint32 sequenceId     = commandTimerData.getSequenceId();
	float warmupMax = commandTimerData.hasTime(MessageQueueCommandTimer::F_warmup) ? commandTimerData.getMaxTime(MessageQueueCommandTimer::F_warmup) : 0.0f;
	float warmup = commandTimerData.hasTime(MessageQueueCommandTimer::F_warmup) ? commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_warmup) : 0.0f;
	float execute = commandTimerData.hasTime(MessageQueueCommandTimer::F_execute) ? commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_execute) : 0.0f;
	float executeMax = commandTimerData.hasTime(MessageQueueCommandTimer::F_execute) ? commandTimerData.getMaxTime(MessageQueueCommandTimer::F_execute) : 0.0f;
	float cooldownMax = commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown) ? commandTimerData.getMaxTime(MessageQueueCommandTimer::F_cooldown) : 0.0f;
	float cooldown = commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown) ? commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_cooldown) : 0.0f;
	float cooldownMax2 = commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown2) ? commandTimerData.getMaxTime(MessageQueueCommandTimer::F_cooldown2) : 0.0f;
	float cooldown2 = commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown2) ? commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_cooldown2) : 0.0f;
	bool hasWarmup = commandTimerData.hasTime(MessageQueueCommandTimer::F_warmup);
	bool hasExecute = commandTimerData.hasTime(MessageQueueCommandTimer::F_execute);
	bool hasCooldown = commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown);
	bool hasCooldownGroup = commandTimerData.hasCooldownGroup();
	bool hasCooldownGroup2 = commandTimerData.hasCooldownGroup2();
	bool hasFailure = commandTimerData.getFailed();
	bool hasFailureRetry = commandTimerData.getFailedRetry();
	bool hasCooldown2 = commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown2);
	DEBUG_REPORT_LOG(true, ("Got timing message %08x:%08x [%d %d %d %d %d %d %d %d %d %d] [%4.2f, %4.2f] [%4.2f, %4.2f] [%4.2f, %4.2f] [%4.2f, %4.2f]\n",
		commandNameCrc,
		sequenceId,
		hasWarmup,
		hasExecute,
		hasCooldown,
		hasCooldownGroup,
		hasFailure,
		hasFailureRetry,
		commandTimerData.getCooldownGroup(),
		hasCooldown2,
		hasCooldownGroup2,
		commandTimerData.getCooldownGroup2(),
		warmup,
		warmupMax,
		execute, executeMax,
		cooldown, cooldownMax,
		cooldown2, cooldownMax2));
#endif

	if(commandTimerData.getSequenceId() == 0)
	{
		if(commandNameCrc == 0)
		{
			//Assume it's a cooldown-group-only message
			processCooldownGroupOnlyMessage(commandTimerData);
		}
		//Ignore the other kind; they're bogus command queue items
		return;
	}

	if(commandTimerData.hasCooldownGroup())
	{
		m_executingGroup = commandTimerData.getCooldownGroup();
	}
	if(commandTimerData.getFailed() || commandTimerData.getFailedRetry())
	{
		setCommandFailed(commandNameCrc);

		CuiSoundManager::play (CuiSounds::combat_action_failed);
	}

	if(commandTimerData.hasTime(MessageQueueCommandTimer::F_warmup))
	{
		m_warmupMaxTimer = commandTimerData.getMaxTime(MessageQueueCommandTimer::F_warmup);
		m_warmupTimer = commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_warmup);
		setCommandExecuting(commandNameCrc);

		CuiSoundManager::play (CuiSounds::combat_action_warmup);
	}
	if(commandTimerData.hasTime(MessageQueueCommandTimer::F_execute))
	{
		m_executeMaxTimer = commandTimerData.getMaxTime(MessageQueueCommandTimer::F_execute);
		m_executeTimer = commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_execute);
		if(m_executeMaxTimer < ConfigClientGame::getMinimumShowExecuteTime())
		{
			m_executeMaxTimer = 0.0f;
			m_executeTimer = 0.0f;
		}
		if(m_whiteFlashPage)
		{
			doWhiteFlash(commandNameCrc);
		}
		if(commandNameCrc != m_clientOverrideCurrentActionCrc)
		{
			setCommandExecuting(commandNameCrc);
		}

		//CuiSoundManager::play (CuiSounds::combat_action_ok);
	}
	if(commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown))
	{
		if(commandTimerData.hasCooldownGroup())
		{
			m_lastCooldownGroupReceived = m_executingGroup;
		}
		else
		{
			//return;
			//Received timer data with a cooldown timer but no cooldown group, has no meaning.
			//For now we'll use the last command group we received
			m_executingGroup = m_lastCooldownGroupReceived;
		}

		float cooldownMaxTimer = commandTimerData.getMaxTime(MessageQueueCommandTimer::F_cooldown);

		float cooldownTimer = commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_cooldown);

		if(m_executeMaxTimer > 0.0f)
		{
			cooldownMaxTimer += m_executeMaxTimer;
			cooldownTimer += m_executeTimer;
		}

		setCooldownTime(m_executingGroup, cooldownTimer, cooldownMaxTimer);

		if(commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown2))
		{
			float cooldownMaxTimer2 = commandTimerData.getMaxTime(MessageQueueCommandTimer::F_cooldown2);
			float cooldownTimer2 = commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_cooldown2);

			if(m_executeMaxTimer > 0.0f)
			{
				cooldownMaxTimer2 += m_executeMaxTimer;
				cooldownTimer2 += m_executeTimer;
			}

			setCooldownTime(commandTimerData.getCooldownGroup2(), cooldownTimer2, cooldownMaxTimer2);
		}
	}


	m_clientOverrideCooldownGroup = -1;
	m_clientOverrideCurrentActionCrc = 0;
	m_clientOverrideNextActionCommandCrc = 0;
} //lint !e1762 function could be const (no it couldn't it must have this signature for the transceiver)

//----------------------------------------------------------------------

void SwgCuiSideToolbar::processCooldownGroupOnlyMessage(const MessageQueueCommandTimer & commandTimerData)
{
	if(commandTimerData.hasCooldownGroup())
	{
		if(commandTimerData.hasTime(MessageQueueCommandTimer::F_cooldown))
		{
			ClientCommandQueue::setCommandCooldown(commandTimerData.getCooldownGroup(),
				commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_cooldown),
				commandTimerData.getMaxTime(MessageQueueCommandTimer::F_cooldown), false);
			setCooldownTime(commandTimerData.getCooldownGroup(),
				commandTimerData.getCurrentTime(MessageQueueCommandTimer::F_cooldown),
				commandTimerData.getMaxTime(MessageQueueCommandTimer::F_cooldown));
		}
	}
	return;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::setCommandExecuting(const uint32 commandCrc)
{
	if(commandCrc != m_clientOverrideCurrentActionCrc)
	{
		m_clientOverrideCurrentActionCrc = 0;
		resetAllPagesInPool(m_currentActionPages, m_nextCurrentActionPage);
	}
	m_commandExecutingCrc = commandCrc;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::setCommandExecuting(const uint32 commandCrc, float currentExecTime, float maxExecTime)
{
	if(commandCrc != m_clientOverrideCurrentActionCrc)
	{
		m_clientOverrideCurrentActionCrc = 0;
		resetAllPagesInPool(m_currentActionPages, m_nextCurrentActionPage);
	}
	m_commandExecutingCrc = commandCrc;
	m_executeTimer = currentExecTime;
	m_executeMaxTimer = maxExecTime;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::setCommandFailed(const uint32 commandCrc)
{
	m_commandFailedCrc = commandCrc;
	if (commandCrc == m_commandExecutingCrc)
	{
		m_executeMaxTimer = 0;
		m_warmupMaxTimer = 0;
	}
}

//----------------------------------------------------------------------

UIPage *SwgCuiSideToolbar::getPageFromPool(UIPage *original, std::vector<UIPage *> &duplicates, int &index, bool toOriginalParent)
{
	if(index >= static_cast<int>(duplicates.size()))
	{
		UIPage * const dupe = NON_NULL (safe_cast<UIPage *>(original->DuplicateObject ()));
		UIPage &parent = toOriginalParent ? *(safe_cast<UIPage *>(original->GetParent())) : getPage();
		parent.AddChild (dupe);
		parent.MoveChild (dupe, UIBaseObject::Top);
		dupe->SetOpacity(0.8f);
		dupe->SetBackgroundOpacity(0.8f);
		duplicates.push_back(dupe);
		index = duplicates.size();
		dupe->Link();
		parent.Pack();
		return dupe;
	}
	else
	{
		return duplicates[index++];
	}
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::resetAllPagesInPool(std::vector<UIPage *> const &duplicates, int &index)
{
	for(std::vector<UIPage *>::const_iterator i = duplicates.begin(); i != duplicates.end(); ++i)
	{
		UIPage *page = *i;
		page->SetVisible(false);
	}
	index = 0;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::deleteAllPagesInPool(std::vector<UIPage *> &duplicates)
{
	for(std::vector<UIPage *>::iterator i = duplicates.begin(); i != duplicates.end(); ++i)
	{
		UIPage *page = *i;
		page->GetParent()->RemoveChild(page);
	}
	duplicates.clear();
}

//----------------------------------------------------------------------

//Note: this code mirrors the function canUseWeaponWithAbility inside combat.scriptlib
bool SwgCuiSideToolbar::getWeaponTypeIncompatible(uint32 weaponTypesValid, uint32 weaponTypesInvalid, int weaponType)
{
	return !WeaponObject::weaponTypeSatisfies(static_cast<WeaponObject::WeaponType>(weaponType), weaponTypesValid, weaponTypesInvalid);
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbar::isMeleeWeapon(int weaponType)
{
	switch (weaponType)
	{
		case Command::CWT_Rifle:
		case Command::CWT_Carbine:
		case Command::CWT_Pistol:
		case Command::CWT_Heavy:
		case Command::CWT_Thrown:
		case Command::CWT_1handLightsaber:
		case Command::CWT_2handLightsaber:
		case Command::CWT_PolearmLightsaber:
				return false;

		case Command::CWT_1handMelee:
		case Command::CWT_2handMelee:
		case Command::CWT_Unarmed:
		case Command::CWT_Polearm:
				return true;
	}

	return false;
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbarNamespace::isRangeOkay(float minRange, float maxRange, Command const & command)
{
	CreatureObject * const sourceObj = Game::getPlayerCreature();
	if (!sourceObj)
		return true;

	Object const * targetObj = 0;
	if (CuiPreferences::getAutoAimToggle())
		targetObj = NetworkIdManager::getObjectById(sourceObj->getIntendedTarget());
	else
		targetObj = NetworkIdManager::getObjectById(sourceObj->getLookAtTarget());

	if (!targetObj)
		return true;

	if (command.m_targetType == Command::CTT_Optional)
	{
		CreatureObject const * const targetCreature = CreatureObject::asCreatureObject(targetObj);

		if (!ClientCommandChecks::canDoBeneficialCommandOnTarget(targetCreature) &&
			CombatDataTable::getValidTarget(command.m_commandHash) == CombatDataTable::VTT_friend)
			return true;
	}

	if(command.m_targetType == Command::CTT_All)
		return true;

	float const collisionSphereDistance = sourceObj->getDistanceBetweenCollisionSpheres_w(*targetObj);;

	if (minRange > 0 && collisionSphereDistance < minRange)
		return false;

	if (maxRange > 0 && collisionSphereDistance > maxRange)
		return false;

	return true;
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbar::isRangedWeapon(int weaponType)
{
	switch (weaponType)
	{
		case Command::CWT_Rifle:
		case Command::CWT_Carbine:
		case Command::CWT_Pistol:
		case Command::CWT_Heavy:
			return true;

		case Command::CWT_Thrown:
		case Command::CWT_1handLightsaber:
		case Command::CWT_2handLightsaber:
		case Command::CWT_PolearmLightsaber:
		case Command::CWT_1handMelee:
		case Command::CWT_2handMelee:
		case Command::CWT_Unarmed:
		case Command::CWT_Polearm:
				return false;
	}

	return false;
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbar::isLightsaberWeapon(int weaponType)
{
	switch (weaponType)
	{
		case Command::CWT_1handLightsaber:
		case Command::CWT_2handLightsaber:
		case Command::CWT_PolearmLightsaber:
			return true;

		case Command::CWT_Rifle:
		case Command::CWT_Carbine:
		case Command::CWT_Pistol:
		case Command::CWT_Heavy:
		case Command::CWT_Thrown:
		case Command::CWT_1handMelee:
		case Command::CWT_2handMelee:
		case Command::CWT_Unarmed:
		case Command::CWT_Polearm:
			return false;
	}
	return false;
}

//----------------------------------------------------------------------

SwgCuiSideToolbar* SwgCuiSideToolbar::getActiveToolbar()
{
	return s_activeToolbar;
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::clearCommandDisplayStates()
{
	m_clientOverrideCooldownGroup = -1;
	m_clientOverrideCurrentActionCrc = 0;
	m_clientOverrideNextActionCommandCrc = 0;
	m_commandExecutingCrc = 0;
	m_commandFailedCrc = 0;
	m_lastCooldownGroupReceived = -1;
	m_executingGroup = -1;

	resetAllPagesInPool(m_currentActionPages, m_nextCurrentActionPage);
	resetAllPagesInPool(m_failedActionPages, m_nextFailedActionPage);
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::removeOldStyleIcons()
{
	if(m_removedOldIcons) //EDIT
		return;
	m_removedOldIcons = true;

	UIImageStyle * fallback = CuiIconManager::getFallback();
	for(int pane = 0; pane < DEFAULT_PANE_COUNT; ++pane)
	{
		for(int slot = 24; slot < DEFAULT_ITEM_COUNT_PER_PANE; ++slot)
		{
			CuiDragInfo *item = getToolbarItem(pane, slot);

			if ((item != 0) && (item->type != CuiDragInfoTypes::CDIT_none))
			{
				UIImageStyle * const imageStyle = CuiIconManager::findIconImageStyle (*item);

				//Skip processing macros
				if(item->type == CuiDragInfoTypes::CDIT_macro)
					continue;

				if(imageStyle && (imageStyle != fallback))
				{
					UIString resourceName;
					imageStyle->GetProperty(UIImageFragment::PropertyName::SourceResource, resourceName);
					if(strstr(Unicode::wideToNarrow(resourceName).c_str(), "ui_rebel_icons") != NULL)
					{
						item->clear();
					}
				}
			}
		}
	}

	repopulateSlots();
	saveSettings();
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::setCooldownTime(int cooldownGroup, float currentTime, float maxTime) const
{

#if 0
	// LWJ Removed: 7/9/2007
	// Removed this fix due to other problems it caused.
	// LWJ Addition: 6/6/2007
	// We will now check to see if this cooldown group already exists.  If it does
	// we will only update the time if it is more than the current time.
	CooldownTimerMap::iterator	iter = s_cooldownTimers.find(std::make_pair(Game::getPlayerNetworkId(), cooldownGroup));

	if(iter != s_cooldownTimers.end())
	{
		CooldownTimerMapValue value = (*iter).second;

		float	valCurrent	= value.second / 1000.f,
				valMax		= value.first / 1000.f;

		if(valCurrent < currentTime && valMax == maxTime)
		{
			s_cooldownTimers[std::make_pair(Game::getPlayerNetworkId(), cooldownGroup)] = std::make_pair(static_cast<unsigned long>(maxTime * 1000.0f), static_cast<unsigned long>(currentTime * 1000.0f));
		}
		else
		{
			// Make sure we call setCommandCooldown() with the right values
			currentTime		= valCurrent;
			maxTime			= valMax;
		}
	}
	else
	{
		s_cooldownTimers[std::make_pair(Game::getPlayerNetworkId(), cooldownGroup)] = std::make_pair(static_cast<unsigned long>(maxTime * 1000.0f), static_cast<unsigned long>(currentTime * 1000.0f));
	}

#if defined(_DEBUG)
	DEBUG_REPORT_LOG(true, ("UI: Calling SwgCuiSideToolbar::setCooldownTime(), cooldown group %d, current time %f, max time %f\n",
		cooldownGroup,currentTime,maxTime));
#endif	// #if defined(_DEBUG)

	// end LWJ Addition
#else
#if defined(_DEBUG)
	DEBUG_REPORT_LOG(true, ("UI: Calling SwgCuiSideToolbar::setCooldownTime(), cooldown group %d, current time %f, max time %f\n",
		cooldownGroup,currentTime,maxTime));
#endif	// #if defined(_DEBUG)
	s_cooldownTimers[std::make_pair(Game::getPlayerNetworkId(), cooldownGroup)] = std::make_pair(static_cast<unsigned long>(maxTime * 1000.0f), static_cast<unsigned long>(currentTime * 1000.0f));
#endif

	ClientCommandQueue::setCommandCooldown(cooldownGroup, currentTime, maxTime, false);
}

//----------------------------------------------------------------------

bool SwgCuiSideToolbar::isCooldownFinished(int cooldownGroup) const
{
	CooldownTimerMap::const_iterator it = s_cooldownTimers.find(std::make_pair(Game::getPlayerNetworkId(), cooldownGroup));
	if(it != s_cooldownTimers.end())
	{
		if (it->second.second < it->second.first)
		{
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------

// Returns a value from 0 to 100, with 100 being no progress (100% left) and 0 meaning done (0% left)
unsigned long SwgCuiSideToolbar::getCooldownProgress (int cooldownGroup) const
{
	CooldownTimerMap::const_iterator it = s_cooldownTimers.find(std::make_pair(Game::getPlayerNetworkId(), cooldownGroup));

	if (it != s_cooldownTimers.end())
	{
		const unsigned long currentMaxTimer = it->second.first;
		const unsigned long currentTimer = it->second.second;

		if ((currentTimer >= currentMaxTimer) || (currentMaxTimer == 0))
		{
			return 0;
		}

		return ((currentMaxTimer - currentTimer) * 100) / currentMaxTimer;
	}

	return 0;
}

//----------------------------------------------------------------------

//Returns 0 if there is no object or no command for that object
uint32 SwgCuiSideToolbar::getTemplateCrcForObject(const CuiDragInfo & item)
{
	if(item.type != CuiDragInfoTypes::CDIT_object)
		return 0;

	ClientObject * const object = item.getClientObject ();
	if(!object)
		return 0;

	const ObjectTemplate * templ = object->getObjectTemplate();
	if(!templ)
		return 0;

	const CrcString & templateCrc = templ->getCrcName();
	return templateCrc.getCrc();
}

//----------------------------------------------------------------------

void SwgCuiSideToolbar::setDefaultAction(int slot, bool activateActionIfNeeded)
{
	// Currently only ground toolbars can set default actions

	if (m_sceneType == Game::ST_ground)
	{
		std::string const actionString = getActionString(slot);

		if (actionString.empty())
		{
			// They selected an empty slot
			return;
		}

		ClientObject * object = getObjectFromSlot(slot);

		if (actionString == s_objectKey && !canBeSlotted(object))
		{
			if(!CuiPreferences::getScrollThroughDefaultActions())
				performToolbarAction(slot);
			else if(m_clickedInActionBar)
				performToolbarAction(slot);

			m_clickedInActionBar = false;

			return;
		}

		m_defaultActionSlot = slot;

		GroundCombatActionManager::setCurrentSecondaryAction(getCommandName(m_defaultActionSlot), object, 1); //we use 1 here to signify we're coming from side toolbar
		if(activateActionIfNeeded && CuiPreferences::getCanFireSecondariesFromToolbar())
		{
			GroundCombatActionManager::attemptAction(GroundCombatActionManager::AT_secondaryAttackFromToolbar);
		}

		saveSettings();
		CuiSettings::save();
	}
	else
	{
		performToolbarAction(slot);
	}
}

//----------------------------------------------------------------------

std::string SwgCuiSideToolbar::getActionString(int const slot)
{
	std::string result;

	if (slot >= 0)
	{
		CuiDragInfo const * const toolbarItem = getToolbarItem(slot);

		if (toolbarItem != NULL)
		{
			std::string compareString;
			if(toolbarItem->type == CuiDragInfoTypes::CDIT_command)
			{
				if (!toolbarItem->str.empty ())
				{
					result = toolbarItem->str;
				}
				else if (!toolbarItem->cmd.empty ())
				{
					std::string str;
					if (CuiMessageQueueManager::findCommandString (toolbarItem->cmd, str, false))
						result = str;
				}
			}
			else if (toolbarItem->type == CuiDragInfoTypes::CDIT_object)
			{
				result = s_objectKey;
			}
			else if (toolbarItem->type == CuiDragInfoTypes::CDIT_macro)
			{
				result = s_macroKey;
			}
		}
	}

	if (!result.empty() && result[0] == '/')
		result = result.substr(1);

	chopOff(result);

	return result;
}

//----------------------------------------------------------------------

std::string SwgCuiSideToolbar::getCommandName(int const slot)
{
	std::string result;

	if (slot >= 0)
	{
		CuiDragInfo const * const toolbarItem = getToolbarItem(slot);

		if (toolbarItem != NULL)
		{
			std::string compareString;
			if(toolbarItem->type == CuiDragInfoTypes::CDIT_command)
			{
				if (!toolbarItem->str.empty ())
				{
					result = toolbarItem->str;
				}
				else if (!toolbarItem->cmd.empty ())
				{
					std::string str;
					if (CuiMessageQueueManager::findCommandString (toolbarItem->cmd, str, false))
						result = str;
				}
			}
			else if (toolbarItem->type == CuiDragInfoTypes::CDIT_object)
			{
				UIWidget * const parent = getToolbarItemWidget (slot);
				assert (parent);
				int commandCrc = 0;
				if(parent != 0 && parent->HasProperty(OBJECT_COMMAND_CRC))
				{
					parent->GetPropertyInteger(OBJECT_COMMAND_CRC, commandCrc);
					if(commandCrc != 0)
					{
						std::string str;
						const Command & cmd = CommandTable::getCommand (commandCrc);
						if(!cmd.isNull())
						{
							result = cmd.m_commandName;
						}
					}
				}
			}
			else if (toolbarItem->type == CuiDragInfoTypes::CDIT_macro)
			{
				result = s_macroKey;
			}
		}
	}

	if (!result.empty() && result[0] == '/')
		result = result.substr(1);

	return result;
}

//----------------------------------------------------------------------

ClientObject * SwgCuiSideToolbar::getObjectFromSlot(int const slot)
{
	if (slot >= 0)
	{
		CuiDragInfo * toolbarItem = getToolbarItem(slot);

		if (toolbarItem != NULL)
		{
			if (toolbarItem->type == CuiDragInfoTypes::CDIT_object)
			{
				long const activeTab = m_tabs->GetActiveTab();
				CuiDragInfo * info = getToolbarItem(activeTab, slot);
				ClientObject * object = (info != 0) ? info->getClientObject() : 0;

				return object;
			}
		}
	}

	return NULL;
}

//-------------------------------------------------------------------

void SwgCuiSideToolbar::onSceneChanged(bool const &)
{
	s_activeToolbar = NULL;
}

void SwgCuiSideToolbar::onCurrentWeaponChanged(CreatureObject const & creature)
{
	UNREF(creature);
	for(int slot = 24; slot < DEFAULT_ITEM_COUNT_PER_PANE; ++slot)
	{
		updateCommandRange(slot);
	}
}

void SwgCuiSideToolbar::onMaxRangeWeaponChanged(WeaponObject const & weapon)
{
	UNREF(weapon);
	for(int slot = 24; slot < DEFAULT_ITEM_COUNT_PER_PANE; ++slot)
	{
		updateCommandRange(slot);
	}
}

void SwgCuiSideToolbar::onMinRangeWeaponChanged(WeaponObject const & weapon)
{
	UNREF(weapon);
	for(int slot = 24; slot < DEFAULT_ITEM_COUNT_PER_PANE; ++slot)
	{
		updateCommandRange(slot);
	}
}

void SwgCuiSideToolbar::updateCommandRange(int slot)
{
	CreatureObject *creature = Game::getPlayerCreature();
	CuiDragInfo *item = getToolbarItem(slot);
	UIWidget *widget = getToolbarItemWidget(slot);
	if(!widget)
		return;
	const WeaponObject * weapon = 0;

	Command cmd;
	if(item->type == CuiDragInfoTypes::CDIT_command)
	{
		if (!item->cmd.empty ())
			cmd = CommandTable::getCommand (Crc::normalizeAndCalculate(item->cmd.c_str()));
		else if (!item->str.empty())
			cmd = CommandTable::getCommand (Crc::normalizeAndCalculate(item->str.c_str()));
	}
	if(item->type == CuiDragInfoTypes::CDIT_object)
	{
		int commandCrc = 0;
		if(widget->HasProperty(OBJECT_COMMAND_CRC))
		{
			widget->GetPropertyInteger(OBJECT_COMMAND_CRC, commandCrc);
			if(commandCrc != 0)
				cmd = CommandTable::getCommand (commandCrc);
			weapon = dynamic_cast<WeaponObject *>(item->getClientObject());
		}
	}

	if (!cmd.isNull())
	{
		bool weaponCheckLast = false;
		if (!weapon && creature)
			weapon = creature->getCurrentWeapon();
		else
			weaponCheckLast = true;
		float minWeaponRange = weapon ? weapon->getMinRange() : -1.0f;
		float maxWeaponRange = weapon ? weapon->getMaxRange() : -1.0f;

		if (weapon)
			CommandChecks::getRangeForCommand(&cmd, minWeaponRange, maxWeaponRange, weaponCheckLast, minWeaponRange, maxWeaponRange);
		else
			CommandChecks::getRangeForCommand(&cmd, -1, -1, false, minWeaponRange, maxWeaponRange);

		bool needRangeCheck = true;
		// If a target is truly optional we don't need a range check...
		if (cmd.m_targetType == Command::CTT_Optional &&
			CombatDataTable::getValidTarget(cmd.m_commandHash) == CombatDataTable::VTT_none)
			needRangeCheck = false;

		// range is always valid if no target, or target not allowed
		if (cmd.m_targetType == Command::CTT_None || cmd.m_targetType == Command::CTT_Location || cmd.m_targetType == Command::CTT_All)
			needRangeCheck = false;

		if (needRangeCheck && (minWeaponRange != -1 || minWeaponRange != -1))
		{
			widget->SetPropertyFloat(MIN_RANGE, minWeaponRange);
			widget->SetPropertyFloat(MAX_RANGE, maxWeaponRange);
		}
		else
		{
			widget->RemoveProperty(MIN_RANGE);
			widget->RemoveProperty(MAX_RANGE);
		}
	}
}

//-------------------------------------------------------------------

bool SwgCuiSideToolbar::isSideToolbar() const
{
	return m_sideToolbar;
}

//-------------------------------------------------------------------

Game::SceneType SwgCuiSideToolbar::getSceneType() const
{
	return m_sceneType;
}

//-------------------------------------------------------------------

void SwgCuiSideToolbar::updateVisibility()
{
	bool visible = CuiPreferences::getUseSideToolbar();
	if (m_volumePage)
		m_volumePage->SetVisible(visible);
	if (m_volumeBackgroundPage)
		m_volumeBackgroundPage->SetVisible(visible);
	if (m_volumeTimersPage)
		m_volumeTimersPage->SetVisible(visible);
	if (m_toolbarPage)
		m_toolbarPage->SetVisible(visible);
	if (visible)
	{
		repopulateSlots();
		startProcessingActions();
	}
	else {
		stopProcessingActions();
	}
	m_sideToolbar = visible;
}

void SwgCuiSideToolbar::stopProcessingActions()
{
	if(m_action)
	{
		CuiActionManager::removeAction (m_action);
		delete m_action;
		m_action = 0;
	}
}

//-------------------------------------------------------------------

void SwgCuiSideToolbar::startProcessingActions()
{
	if(m_action)
		return;

	m_action = new SwgCuiSideToolbarAction (this);

	CuiActionManager::addAction (CuiActions::toolbarSlot24,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot25,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot26,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot27,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot28,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot29,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot30,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot31,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot32,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot33,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot34,	   m_action, false);
	CuiActionManager::addAction (CuiActions::toolbarSlot35,	   m_action, false);
}
//======================================================================
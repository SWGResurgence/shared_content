//======================================================================
//
// SwgCuiSideToolbar.h
// copyright (c) 2001 Sony Online Entertainment
//
//======================================================================

#ifndef INCLUDED_SwgCuiSideToolbar_H
#define INCLUDED_SwgCuiSideToolbar_H

//======================================================================

#include "clientGame/ClientCommandQueue.h"
#include "clientGame/Game.h"
#include "sharedFoundation/NetworkId.h"
#include "sharedMessageDispatch/Receiver.h"
#include "sharedObject/CachedNetworkId.h"
#include "swgClientUserInterface/SwgCuiLockableMediator.h"
#include "UITypes.h"
#include "UILowerString.h"

#include <map>
#include <vector>


//----------------------------------------------------------------------

class CreatureObject;
class CuiDragInfo;
class CuiWidget3dObjectListViewer;
class MessageQueueCommandTimer;
class PlayerObject;
class SwgCuiMfdStatus;
class UIButton;
class UIColorEffector;
class UIImage;
class UIPage;
class UIPie;
class UIScrollbar;
class UITabbedPane;
class UIVolumePage;
class UIText;
class WeaponObject;

namespace MessageDispatch
{
	class Emitter;
	class Callback;
};

//----------------------------------------------------------------------

class SwgCuiSideToolbar :
public SwgCuiLockableMediator,
public MessageDispatch::Receiver
{
public:

	explicit            SwgCuiSideToolbar (UIPage & page, Game::SceneType sceneType);

	static void install();
	static void performSecondaryAttack(NetworkId const & target);

	virtual bool        OnMessage                    (UIWidget *context, const UIMessage & msg );
	virtual void        OnPopupMenuSelection         (UIWidget * context);
	virtual void        OnVolumePageSelectionChanged (UIWidget * context);

	virtual void        receiveMessage               (const MessageDispatch::Emitter & source, const MessageDispatch::MessageBase & message);

			int         getCurrentPane               ();

	        bool        performToolbarAction         (int slot);

			void        update                       (float deltaTimeSecs);
	virtual void        saveSettings                 () const;
	virtual void        loadSettings                 ();

	void                onMacrosChangedReset         (const bool & );

	void                onPlayerSetup                (CreatureObject const & payload);
	void                updateKeyBindings            ();

	void                doWhiteFlash                 (const uint32 strCrc);
	void                onCommandTimerDataReceived   (const MessageQueueCommandTimer & commandTimerData);

	void                setCommandExecuting          (const uint32 strCrc);
	void                setCommandExecuting          (const uint32 strCrc, float currentExecTime, float maxExecTime);
	void                setCommandFailed             (const uint32 strCrc);

	static SwgCuiSideToolbar* getActiveToolbar           ();

	void                clearCommandDisplayStates    ();  // Clears active, pending, and failed boxes

	void setDefaultAction(int slot, bool activateActionIfNeeded = true);

	void                setCooldownTime              (int cooldownGroup, float currentTime, float maxTime) const;
	bool                isSideToolbar() const;
	void                startProcessingActions       ();
	void                stopProcessingActions        ();
	void                updateVisibility             ();

protected:
	virtual void        performActivate              ();
	virtual void        performDeactivate            ();

	void                setupDefaults                ();

private:
	                   ~SwgCuiSideToolbar                ();

	CuiDragInfo *       getToolbarItem               (int pane, int slot);
	CuiDragInfo *       getToolbarItem               (int slot);

	UIWidget *          createToolbarWidget          (const CuiDragInfo & item);

	int                 discoverToolbarSlot          (UIWidget * context);

	void                setToolbarItem               (int pane, int slot, const CuiDragInfo & item);

	UIWidget *          getToolbarItemWidget         (int slot);
	UIWidget *          getToolbarItemShadeWidget    (int slot);
	UIWidget *          getToolbarItemBackgroundWidget (int slot);
	void                populateSlot                 (int slot);

	void                repopulateSlots              ();

	void                onCommandRemoved             (const std::pair<CreatureObject *, std::string> &);

	void                onSceneChanged               (bool const &);

	void				onCurrentWeaponChanged		 (CreatureObject const & creature);
	void				onMaxRangeWeaponChanged		 (WeaponObject const & weapon);
	void				onMinRangeWeaponChanged		 (WeaponObject const & weapon);
	void                updateCommandRange           (int slot);

	void                clearWidgets                 ();

	void                setupTabData                 ();
	void                updateFromSizes              ();

	UIPage *            getPageFromPool              (UIPage *original, stdvector<UIPage *>::fwd &duplicates, int &index, bool toOriginalParent = false);
	void                resetAllPagesInPool          (stdvector<UIPage *>::fwd const &duplicates, int &index);
	void                deleteAllPagesInPool         (stdvector<UIPage *>::fwd &duplicates);

	bool                getWeaponTypeIncompatible    (uint32 weaponTypesValid, uint32 weaponTypesInvalid, int weaponType);
	bool                isMeleeWeapon                (int weaponType);
	bool                isRangedWeapon               (int weaponType);
	bool                isLightsaberWeapon           (int weaponType);

	void                removeOldStyleIcons          ();

	bool                isCooldownFinished           (int cooldownGroup) const;
	unsigned long       getCooldownProgress          (int cooldownGroup) const;

	uint32				getTemplateCrcForObject       (const CuiDragInfo & item);

	void                processCooldownGroupOnlyMessage(const MessageQueueCommandTimer & commandTimerData);
	                    SwgCuiSideToolbar                ();
	                    SwgCuiSideToolbar                (const SwgCuiSideToolbar &);
	SwgCuiSideToolbar &     operator=                    (const SwgCuiSideToolbar &);

	std::string getActionString(int slot);
	std::string getCommandName(int const slot);
	ClientObject * getObjectFromSlot(int const slot);

	Game::SceneType     getSceneType                  () const;

private:
	class SwgCuiSideToolbarAction;
	typedef stdvector<CuiDragInfo>::fwd     ToolbarItemPane;
	typedef stdvector<ToolbarItemPane>::fwd ToolbarItemPaneVector;

	UIPage *					  m_toolbarPage;

	UIVolumePage *                m_volumePage;
	UIVolumePage *                m_volumeBackgroundPage;
	UIPage *                      m_volumeTimersPage;

	UITabbedPane *                m_tabs;

	SwgCuiSideToolbarAction *         m_action;

	ToolbarItemPaneVector *       m_toolbarItemPanes;

	long                          m_draggingPane;
	long                          m_draggingSlot;
	long                          m_dragCounter;

	UIPoint                       m_popupPoint;
	long                          m_popupSlot;

	UIPage *                      m_sampleItemPage;

	MessageDispatch::Callback *   m_callback;

	UIRect                        m_iconMargin;

	UIColorEffector *             m_effectorUse;
	UIColorEffector *             m_effectorUseChild;

	bool                          m_sendEquipToolbar;

	UIVolumePage *                m_volumeKeyBindings;

	int                           m_mouseDownOnSlot;

	UISize                        m_lastSize;

	UILowerString                 m_palIconFlashColor;

	UIButton *                    m_sampleItemButton;

	bool                          m_playerIsSetup;

	bool						  m_toolbarHasLocalSettings;

	UIPage *                      m_currentActionPage;
	UIPage *                      m_failedActionPage;
	UIPage *                      m_whiteFlashPage;

	std::vector<UIPage *>         m_currentActionPages;
	std::vector<UIPage *>         m_failedActionPages;
	std::vector<UIPage *>         m_whiteFlashPages;

	int                           m_nextNextActionPage;
	int                           m_nextCurrentActionPage;
	int                           m_nextFailedActionPage;
	int                           m_nextWhiteFlashPage;

	float                         m_executeMaxTimer;
	float                         m_executeTimer;

	float                         m_warmupMaxTimer;
	float                         m_warmupTimer;

	Game::SceneType m_sceneType;

	uint32                        m_commandExecutingCrc;
	uint32                        m_commandFailedCrc;

	int                           m_lastCooldownGroupReceived;
	int                           m_executingGroup;

	int                           m_mouseOverSlot;
	UIPage *                      m_mouseOverPage;

	uint32                        m_clientOverrideCurrentActionCrc;
	uint32                        m_clientOverrideNextActionCommandCrc;
	int                           m_clientOverrideCooldownGroup;

	int                           m_defaultActionSlot;
	UIPage *                      m_defaultActionPage;
	int                           m_nextDefaultActionPage;

	UIPage *                      m_wrongWeaponTypePage;
	stdvector<UIPage *>::fwd      m_wrongWeaponTypePages;
	int                           m_nextWrongWeaponTypePage;

	UIEffector *         m_effectorCurrent;

	float *                       m_lastFadedHeight;

	bool                          m_removedOldIcons;

	bool						  m_clickedInActionBar;
	bool						  m_sideToolbar;
};

//======================================================================

#endif
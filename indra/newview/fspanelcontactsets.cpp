/*
 * @file fspanelcontactsets.cpp
 * @brief Contact sets UI
 *
 * (C) 2013 Cinder Roxley @ Second Life <cinder.roxley@phoenixviewer.com>
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "llviewerprecompiledheaders.h"

#include "llnotificationsutil.h"

#include "fspanelcontactsets.h"
#include "fscontactsfloater.h"
#include "llavataractions.h"
#include "llcallingcard.h"
#include "llfloateravatarpicker.h"
#include "llfloaterreg.h"
#include "llslurl.h"
#include "lltrans.h"

#include <boost/foreach.hpp>

static LLRegisterPanelClassWrapper<FSPanelContactSets> t_panel_contact_sets("contact_sets_panel");

FSPanelContactSets::FSPanelContactSets() : LLPanel()
, mContactSetCombo(NULL)
, mAvatarList(NULL)
{
	mContactSetChangedConnection = LGGContactSets::getInstance()->setContactSetChangeCallback(boost::bind(&FSPanelContactSets::updateSets, this, _1));
}

FSPanelContactSets::~FSPanelContactSets()
{
	if (mContactSetChangedConnection.connected())
		mContactSetChangedConnection.disconnect();
}

BOOL FSPanelContactSets::postBuild()
{
	childSetAction("add_set_btn",			boost::bind(&FSPanelContactSets::onClickAddSet,			this));
	childSetAction("remove_set_btn",		boost::bind(&FSPanelContactSets::onClickRemoveSet,		this));
	childSetAction("config_btn",			boost::bind(&FSPanelContactSets::onClickConfigureSet,	this));
	childSetAction("add_btn",				boost::bind(&FSPanelContactSets::onClickAddAvatar,		this));
	childSetAction("remove_btn",			boost::bind(&FSPanelContactSets::onClickRemoveAvatar,	this));
	childSetAction("profile_btn",			boost::bind(&FSPanelContactSets::onClickOpenProfile,	this));
	childSetAction("start_im_btn",			boost::bind(&FSPanelContactSets::onClickStartIM,		this));
	childSetAction("offer_teleport_btn",	boost::bind(&FSPanelContactSets::onClickOfferTeleport,	this));
	
	mContactSetCombo = getChild<LLComboBox>("combo_sets");
	if (mContactSetCombo)
	{
		mContactSetCombo->setCommitCallback(boost::bind(&FSPanelContactSets::refreshSetList, this));
		refreshContactSets();
	}
	
	mAvatarList = getChild<LLAvatarList>("contact_list");
	if (mAvatarList)
	{
		mAvatarList->setCommitCallback(boost::bind(&FSPanelContactSets::onSelectAvatar, this));
		mAvatarList->setNoItemsCommentText(getString("empty_list"));
		generateAvatarList(mContactSetCombo->getSimple());
	}
	
	return TRUE;
}

void FSPanelContactSets::onSelectAvatar()
{
	mAvatarSelections.clear();
	mAvatarList->getSelectedUUIDs(mAvatarSelections);
	resetControls();
}

void FSPanelContactSets::generateAvatarList(const std::string& contact_set)
{
	if (!mAvatarList) return;
	
	uuid_vec_t& avatars = mAvatarList->getIDs();
	avatars.clear();
	if (contact_set == getString("no_sets"))
	{
		
	}
	//else if (contact_set == "Friends")
	//{
	//	LLAvatarTracker::buddy_map_t buddies;
	//	LLAvatarTracker::instance().copyBuddyList(buddies);
	//	LLAvatarTracker::buddy_map_t::const_iterator buddy = buddies.begin();
	//	for (; buddy != buddies.end(); ++buddy)
	//	{
	//		avatars.push_back(buddy->first);
	//	}
	//}
	else
	{
		LGGContactSets::ContactSetGroup* group = LGGContactSets::getInstance()->getGroup(contact_set);	// UGLY!
		BOOST_FOREACH(const LLUUID id, group->mFriends)
		{
			avatars.push_back(id);
		}
	}
	mAvatarList->setDirty();
	resetControls();
}

void FSPanelContactSets::resetControls()
{
	bool has_sets = (!LGGContactSets::getInstance()->getAllGroups().empty());
	bool has_selection = mAvatarSelections.size();
	childSetEnabled("remove_set_btn", has_sets);
	childSetEnabled("config_btn", has_sets);
	childSetEnabled("add_btn", has_sets);
	childSetEnabled("remove_btn", (has_sets && has_selection));
	childSetEnabled("profile_btn", has_selection);
	childSetEnabled("start_im_btn", has_selection);
	childSetEnabled("offer_teleport_btn", has_selection);	// Should probably check if they're online...
}

void FSPanelContactSets::updateSets(LGGContactSets::EContactSetUpdate type)
{
	switch (type)
	{
		case LGGContactSets::UPDATED_LISTS:
			refreshContactSets();
		case LGGContactSets::UPDATED_MEMBERS:
			refreshSetList();
			break;
	}
}

void FSPanelContactSets::refreshContactSets()
{
	if (!mContactSetCombo) return;
	
	mContactSetCombo->clearRows();
	std::vector<std::string> contact_sets = LGGContactSets::getInstance()->getAllGroups();
	if (!contact_sets.empty())
	{
		BOOST_FOREACH(const std::string& set_name, contact_sets)
		{
			mContactSetCombo->add(set_name);
		}
	}
	else
	{
		mContactSetCombo->add(getString("no_sets"), LLSD("No Set"));
	}
	// This only gets enabled for testing.
	//mContactSetCombo->add(LLTrans::getString("InvFolder Friends"), LLSD("Friends"), ADD_TOP);
	resetControls();
}

void FSPanelContactSets::refreshSetList()
{
	generateAvatarList(mContactSetCombo->getSimple());
	resetControls();
}

void FSPanelContactSets::onClickAddAvatar()
{
	LLFloater* root_floater = gFloaterView->getParentFloater(this);
	LLFloater* avatar_picker = LLFloaterAvatarPicker::show(boost::bind(&FSPanelContactSets::handlePickerCallback, this, _1, mContactSetCombo->getSimple()),
														   TRUE, TRUE, TRUE, root_floater->getName());
	if (root_floater && avatar_picker)
		root_floater->addDependentFloater(avatar_picker);
}

void FSPanelContactSets::handlePickerCallback(const uuid_vec_t& ids, const std::string& set)
{
	if (ids.empty() || !mContactSetCombo) return;
	
	BOOST_FOREACH(const LLUUID& id, ids)
	{
		if (!LLAvatarTracker::instance().isBuddy(id))
			LGGContactSets::getInstance()->addNonFriendToList(id);
		LGGContactSets::getInstance()->addFriendToGroup(id, set);
	}
}

void FSPanelContactSets::onClickRemoveAvatar()
{
	if (!(mAvatarList && mContactSetCombo)) return;
	
	std::string set = mContactSetCombo->getSimple();
	BOOST_FOREACH(const LLUUID& id, mAvatarSelections)
	{
		LGGContactSets::getInstance()->removeFriendFromGroup(id, set);
		if (!LLAvatarTracker::instance().isBuddy(id) && LGGContactSets::getInstance()->getFriendGroups(id).size() > 1)
			LGGContactSets::getInstance()->removeNonFriendFromList(id);
	}
}

void FSPanelContactSets::onClickAddSet()
{
	LLNotificationsUtil::add("AddNewContactSet", LLSD(), LLSD(), &handleAddContactSetCallback);
}

void FSPanelContactSets::onClickRemoveSet()
{
	LLSD payload, args;
	std::string set = mContactSetCombo->getSimple();
	args["SET_NAME"] = set;
	payload["contact_set"] = set;
	LLNotificationsUtil::add("RemoveContactSet", args, payload, &handleRemoveContactSetCallback);
}

void FSPanelContactSets::onClickConfigureSet()
{
	LLFloater* root_floater = gFloaterView->getParentFloater(this);
	LLFloater* config_floater = LLFloaterReg::showInstance("fs_contact_set_config", LLSD(mContactSetCombo->getSimple()));
	if (root_floater && config_floater)
		root_floater->addDependentFloater(config_floater);
}

void FSPanelContactSets::onClickOpenProfile()
{
	BOOST_FOREACH(const LLUUID& id, mAvatarSelections)
	{
		LLAvatarActions::showProfile(id);
	}
}

void FSPanelContactSets::onClickStartIM()
{
	if ( mAvatarSelections.size() == 1 )
	{
		LLAvatarActions::startIM(mAvatarSelections[0]);
	}
	else if ( mAvatarSelections.size() > 1 )
	{
		LLAvatarActions::startConference(mAvatarSelections);
	}
}

void FSPanelContactSets::onClickOfferTeleport()
{
	LLAvatarActions::offerTeleport(mAvatarSelections);
}

////////////////////
// Static methods //
////////////////////

// static
bool FSPanelContactSets::handleAddContactSetCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		std::string set_name = response["message"].asString();
		LGGContactSets::getInstance()->addGroup(set_name);
	}
	return false;
}

// static
bool FSPanelContactSets::handleRemoveContactSetCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LGGContactSets::getInstance()->deleteGroup(notification["payload"]["contact_set"].asString());
	}
	return false;
}
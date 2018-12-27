/*
This file is part of Bettergram.

For license and copyright information please follow this link:
https://github.com/bettergram/bettergram/blob/master/LEGAL
*/

#include "dialogs_chat_tabs.h"
#include "dialogs_chat_tab_button.h"
#include "ui/widgets/buttons.h"
#include "styles/style_dialogs.h"
#include "bettergram/bettergramservice.h"

namespace Dialogs {

ChatTabs::ChatTabs(QWidget *parent) : TWidget(parent)
  , _type(EntryType::None)
  ,_favoriteButton(EntryType::Favorite, this, st::dialogsChatTabsFavoriteButton)
  ,_groupButton(EntryType::Group, this, st::dialogsChatTabsGroupButton)
  ,_oneOnOneButton(EntryType::OneOnOne, this, st::dialogsChatTabsOneOnOneButton)
  ,_announcementButton(EntryType::Channel | EntryType::Feed, this, st::dialogsChatTabsAnnouncementButton) {

	_listButtons.push_back(_favoriteButton);
	_listButtons.push_back(_groupButton);
	_listButtons.push_back(_oneOnOneButton);
	_listButtons.push_back(_announcementButton);

	setGeometryToLeft(0, 0, width(), _listButtons.first()->height());

	_favoriteButton->setClickedCallback([this] { onTabClicked(_favoriteButton->type()); });
	_groupButton->setClickedCallback([this] { onTabClicked(_groupButton->type()); });
	_oneOnOneButton->setClickedCallback([this] { onTabClicked(_oneOnOneButton->type()); });
	_announcementButton->setClickedCallback([this] { onTabClicked(_announcementButton->type()); });

	QSettings settings(Bettergram::BettergramService::instance()->bettergramSettingsPath(), QSettings::IniFormat);
	_type = EntryTypes(static_cast<EntryType>(settings.value(qsl("lastTab"), static_cast<unsigned>(EntryType::None)).toUInt()));

   if(_type != EntryType::None)
   {
      selectTab(_type);
      emit tabSelected(_type);
   }
}

void ChatTabs::selectTab(const EntryTypes &type)
{
	_type = type;

	// Set default icons to tab buttons
	_favoriteButton->unselect();
	_groupButton->unselect();
	_oneOnOneButton->unselect();
	_announcementButton->unselect();

	QSettings settings(Bettergram::BettergramService::instance()->bettergramSettingsPath(), QSettings::IniFormat);
	settings.setValue(qsl("lastTab"), static_cast<unsigned>(type));

	// Set highlighted icon to the current tab button

	switch (_type.value()) {
	case static_cast<unsigned>(EntryType::Favorite):
		_favoriteButton->select();
		break;
	case static_cast<unsigned>(EntryType::Group):
		_groupButton->select();
		break;
	case static_cast<unsigned>(EntryType::OneOnOne):
		_oneOnOneButton->select();
		break;
	case static_cast<unsigned>(EntryType::Channel):
	case static_cast<unsigned>(EntryType::Feed):
	case (EntryType::Channel | EntryType::Feed).value():
		_announcementButton->select();
		break;
	case static_cast<unsigned>(EntryType::None):
		break;
	default:
		DEBUG_LOG(("Can not recognize EntryType value '%1'").arg(static_cast<int>(type)));
		break;
	}
}

const EntryTypes &ChatTabs::selectedTab() const
{
	return _type;
}

void ChatTabs::unreadCountChanged(int countInFavorite, int countInGroup, int countInOneOnOne, int countInAnnouncement)
{
	_favoriteButton->setUnreadCount(countInFavorite);
	_groupButton->setUnreadCount(countInGroup);
	_oneOnOneButton->setUnreadCount(countInOneOnOne);
	_announcementButton->setUnreadCount(countInAnnouncement);
}

void ChatTabs::onTabClicked(const EntryTypes &type)
{
	// If user clicks to selected Tab twice
	// this tab becomes unselected and we show all messages without filtering

	if ((_type & type) == type) {
		_type &= ~type;
	} else {
		_type = type;
	}

	selectTab(_type);
	emit tabSelected(_type);
}

void ChatTabs::resizeEvent(QResizeEvent *e) {
	Q_UNUSED(e);
	updateControlsGeometry();
}

void ChatTabs::updateControlsGeometry() {
	//TODO: it would be much better to use QRowLayout for this

	// Align all buttons at the center

	// We assume that all buttons have the same width
	int horizontalSpacing = (width() - _listButtons.size() * _listButtons.first()->width()) / 5;
	int x = horizontalSpacing;
	int y = 0;

	for (Ui::IconButton *button : _listButtons) {
		button->moveToLeft(x, y);
		x += button->width();
		x += horizontalSpacing;
	}
}

} // namespace Dialogs

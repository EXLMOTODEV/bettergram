/*
This file is part of Bettergram.

For license and copyright information please follow this link:
https://github.com/bettergram/bettergram/blob/master/LEGAL
*/
#include "boxes/about_box.h"

#include "lang/lang_keys.h"
#include "mainwidget.h"
#include "mainwindow.h"
#include "boxes/confirm_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "styles/style_boxes.h"
#include "platform/platform_file_utilities.h"
#include "core/click_handler_types.h"
#include "core/update_checker.h"

AboutBox::AboutBox(QWidget *parent)
: _version(this, lng_about_version(lt_version, currentVersionText()), st::aboutVersionLink)
, _text1(this, lang(lng_about_text_1), Ui::FlatLabel::InitType::Rich, st::aboutLabel)
, _text2(this, lang(lng_about_text_2), Ui::FlatLabel::InitType::Rich, st::aboutLabel)
, _text3(this, st::aboutLabel) {
}

void AboutBox::prepare() {
	setTitle([] { return qsl("Bettergram"); });

	addButton(langFactory(lng_close), [this] { closeBox(); });

	const auto linkFilter = [](const ClickHandlerPtr &link, auto button) {
		if (const auto url = dynamic_cast<UrlClickHandler*>(link.get())) {
			url->UrlClickHandler::onClick({ button });
			return false;
		}
		return true;
	};

	_text3->setRichText(lng_about_text_3(lt_faq_open, qsl("[a href=\"%1\"]").arg(telegramFaqLink()), lt_faq_close, qsl("[/a]")));
	_text1->setClickHandlerFilter(linkFilter);
	_text2->setClickHandlerFilter(linkFilter);
	_text3->setClickHandlerFilter(linkFilter);

	_version->setClickedCallback([this] { showVersionHistory(); });

	setDimensions(st::aboutWidth, st::aboutTextTop + _text1->height() + st::aboutSkip + _text2->height() + st::aboutSkip + _text3->height());
}

void AboutBox::resizeEvent(QResizeEvent *e) {
	BoxContent::resizeEvent(e);

	_version->moveToLeft(st::boxPadding.left(), st::aboutVersionTop);
	_text1->moveToLeft(st::boxPadding.left(), st::aboutTextTop);
	_text2->moveToLeft(st::boxPadding.left(), _text1->y() + _text1->height() + st::aboutSkip);
	_text3->moveToLeft(st::boxPadding.left(), _text2->y() + _text2->height() + st::aboutSkip);
}

void AboutBox::showVersionHistory() {
	if (cRealAlphaVersion()) {
		auto url = qsl("https://tdesktop.com/");
		switch (cPlatform()) {
		case dbipWindows: url += qsl("win/%1.zip"); break;
		case dbipMac: url += qsl("mac/%1.zip"); break;
		case dbipMacOld: url += qsl("mac32/%1.zip"); break;
		case dbipLinux32: url += qsl("linux32/%1.tar.xz"); break;
		case dbipLinux64: url += qsl("linux/%1.tar.xz"); break;
		}
		url = url.arg(qsl("talpha%1_%2").arg(cRealAlphaVersion()).arg(Core::countAlphaVersionSignature(cRealAlphaVersion())));

		QApplication::clipboard()->setText(url);

		Ui::show(Box<InformBox>("The link to the current private alpha version of Bettergram was copied to the clipboard."));
	} else {
		QDesktopServices::openUrl(qsl("https://bettergram.io/changelog"));
	}
}

void AboutBox::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
		closeBox();
	} else {
		BoxContent::keyPressEvent(e);
	}
}

QString telegramFaqLink() {
	const auto result = qsl("https://bettergram.io#faq");
	//const auto langpacked = [&](const char *language) {
	//	return result + '/' + language;
	//};
	//const auto current = Lang::Current().id();
	//for (const auto language : { "de", "es", "it", "ko" }) {
	//	if (current.startsWith(QLatin1String(language))) {
	//		return langpacked(language);
	//	}
	//}
	//if (current.startsWith(qstr("pt-br"))) {
	//	return langpacked("br");
	//}
	return result;
}

QString currentVersionText() {
	auto result = QString::fromLatin1(AppVersionStr);
	if (cAlphaVersion()) {
		result += qsl(" alpha %1").arg(cAlphaVersion() % 1000);
	} else if (AppBetaVersion) {
		result += " beta";
	}
	return result;
}

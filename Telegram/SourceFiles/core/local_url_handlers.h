/*
This file is part of Bettergram.

For license and copyright information please follow this link:
https://github.com/bettergram/bettergram/blob/master/LEGAL
*/
#pragma once

namespace qthelp {
class RegularExpressionMatch;
} // namespace qthelp

namespace Core {

struct LocalUrlHandler {
	QString expression;
	Fn<bool(
		const qthelp::RegularExpressionMatch &match,
		const QVariant &context)> handler;
};

const std::vector<LocalUrlHandler> &LocalUrlHandlers();

bool InternalPassportLink(const QString &url);

bool StartUrlRequiresActivate(const QString &url);

} // namespace Core

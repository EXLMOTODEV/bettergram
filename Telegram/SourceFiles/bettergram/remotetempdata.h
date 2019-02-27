#pragma once

#include "abstractremotefile.h"

namespace Bettergram {

/**
 * @brief The RemoteTempData class is used to download QByteArray from remote location and emit signal.
 * It does not store the downloaded data, so it does not take much memory in the most of time.
 */
class RemoteTempData : public AbstractRemoteFile {
	Q_OBJECT

public:
	explicit RemoteTempData(QObject *parent);

	explicit RemoteTempData(const QUrl &link, QObject *parent);

public slots:

signals:
	void downloaded(QByteArray data);

protected:
	bool customIsNeedToDownload() const override;
	void dataDownloaded(const QByteArray &data) override;
	void resetData() override;

private:
};

} // namespace Bettergram

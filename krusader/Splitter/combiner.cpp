/***************************************************************************
                          combiner.cpp  -  description
                             -------------------
    copyright            : (C) 2003 by Csaba Karai
    e-mail               : krusader@users.sourceforge.net
    web site             : http://krusader.sourceforge.net
 ---------------------------------------------------------------------------
  Description
 ***************************************************************************

  A

     db   dD d8888b. db    db .d8888.  .d8b.  d8888b. d88888b d8888b.
     88 ,8P' 88  `8D 88    88 88'  YP d8' `8b 88  `8D 88'     88  `8D
     88,8P   88oobY' 88    88 `8bo.   88ooo88 88   88 88ooooo 88oobY'
     88`8b   88`8b   88    88   `Y8b. 88~~~88 88   88 88~~~~~ 88`8b
     88 `88. 88 `88. 88b  d88 db   8D 88   88 88  .8D 88.     88 `88.
     YP   YD 88   YD ~Y8888P' `8888Y' YP   YP Y8888D' Y88888P 88   YD

                                                     S o u r c e    F i l e

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "combiner.h"
#include "../FileSystem/filesystem.h"

// QtCore
#include <QFileInfo>

#include <KI18n/KLocalizedString>
#include <KIOCore/KFileItem>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KWidgetsAddons/KMessageBox>

//TODO: delete destination file on error
//TODO: cache more than one byte array of data

Combiner::Combiner(QWidget* parent,  QUrl baseURLIn, QUrl destinationURLIn, bool unixNamingIn) :
        QProgressDialog(parent, 0), baseURL(baseURLIn), destinationURL(destinationURLIn),
        hasValidSplitFile(false), fileCounter(0), permissions(-1), receivedSize(0),
        statJob(0), combineReadJob(0), combineWriteJob(0), unixNaming(unixNamingIn)
{
    crcContext = new CRC32();

    splitFile = "";

    setMaximum(100);
    setAutoClose(false);    /* don't close or reset the dialog automatically */
    setAutoReset(false);
    setLabelText("Krusader::Combiner");
    setWindowModality(Qt::WindowModal);

    firstFileIs000 = true; //start with this assumption, will set it to false as soon as .000 isn't found
}

Combiner::~Combiner()
{
    combineAbortJobs();
    delete crcContext;
}

void Combiner::combine()
{
    setWindowTitle(i18n("Krusader::Combining..."));
    setLabelText(i18n("Combining the file %1...", baseURL.toDisplayString(QUrl::PreferLocalFile)));

    /* check whether the .crc file exists */
    splURL = baseURL.adjusted(QUrl::RemoveFilename);
    splURL.setPath(splURL.path() + baseURL.fileName() + ".crc");
    KFileItem file(splURL);
    //FIXME: works only for local files - use KIO::stat() instead
    file.refresh();

    if (!file.isReadable()) {
        int ret = KMessageBox::questionYesNo(0, i18n("The CRC information file (%1) is missing.\n"
                                             "Validity checking is impossible without it. Continue combining?",
                                             splURL.toDisplayString(QUrl::PreferLocalFile)));

        if (ret == KMessageBox::No) {
            emit reject();
            return;
        }

        statDest();
    } else {
        permissions = file.permissions() | QFile::WriteUser;

        combineReadJob = KIO::get(splURL, KIO::NoReload, KIO::HideProgressInfo);

        connect(combineReadJob, SIGNAL(data(KIO::Job*,QByteArray)),
                this, SLOT(combineSplitFileDataReceived(KIO::Job*,QByteArray)));
        connect(combineReadJob, SIGNAL(result(KJob*)),
                this, SLOT(combineSplitFileFinished(KJob*)));
    }

    exec();
}

void Combiner::combineSplitFileDataReceived(KIO::Job *, const QByteArray &byteArray)
{
    splitFile += QString(byteArray);
}

void Combiner::combineSplitFileFinished(KJob *job)
{
    combineReadJob = 0;
    QString error;

    if (job->error())
        error = i18n("Error at reading the CRC file (%1).", splURL.toDisplayString(QUrl::PreferLocalFile));
    else {
        splitFile.remove('\r');   // Windows compatibility
        QStringList splitFileContent = splitFile.split('\n');

        bool hasFileName = false, hasSize = false, hasCrc = false;

        for (int i = 0; i != splitFileContent.count(); i++) {
            int ndx = splitFileContent[i].indexOf('=');
            if (ndx == -1)
                continue;
            QString token = splitFileContent[i].left(ndx).trimmed();
            QString value = splitFileContent[i].mid(ndx + 1);

            if (token == "filename") {
                expectedFileName = value;
                hasFileName = true;
            } else if (token == "size") {
                //FIXME - don't use c functions !!!
                sscanf(value.trimmed().toLocal8Bit(), "%llu", &expectedSize);
                hasSize = true;
            }
            if (token == "crc32") {
                expectedCrcSum   = value.trimmed().rightJustified(8, '0');
                hasCrc = true;
            }
        }

        if (!hasFileName || !hasSize || !hasCrc)
            error = i18n("Not a valid CRC file.");
        else
            hasValidSplitFile = true;
    }

    if (!error.isEmpty()) {
        int ret = KMessageBox::questionYesNo(0,
                                             error + i18n("\nValidity checking is impossible without a good CRC file. Continue combining?"));
        if (ret == KMessageBox::No) {
            emit reject();
            return;
        }
    }

    statDest();
}

void Combiner::statDest()
{
    if (writeURL.isEmpty()) {
        writeURL = FileSystem::ensureTrailingSlash(destinationURL);
        if (hasValidSplitFile)
            writeURL.setPath(writeURL.path() + expectedFileName);
        else if (unixNaming)
            writeURL.setPath(writeURL.path() + baseURL.fileName() + ".out");
        else
            writeURL.setPath(writeURL.path() + baseURL.fileName());
    }

    statJob = KIO::stat(writeURL, KIO::StatJob::DestinationSide, 0, KIO::HideProgressInfo);
    connect(statJob, SIGNAL(result(KJob*)), SLOT(statDestResult(KJob*)));
}

void Combiner::statDestResult(KJob* job)
{
    statJob = 0;

    if (job->error()) {
        if (job->error() == KIO::ERR_DOES_NOT_EXIST) {
            openNextFile();
        } else {
            static_cast<KIO::Job*>(job)->uiDelegate()->showErrorMessage();
            emit reject();
        }
    } else { // destination already exists
        KIO::RenameDialog_Options mode = static_cast<KIO::StatJob*>(job)->statResult().isDir() ?
            KIO::RenameDialog_IsDirectory : KIO::RenameDialog_Overwrite;
        KIO::RenameDialog dlg(this, i18n("File Already Exists"), QUrl(), writeURL, mode);
        switch (dlg.exec()) {
        case KIO::R_OVERWRITE:
            openNextFile();
            break;
        case KIO::R_RENAME: {
            writeURL = dlg.newDestUrl();
            statDest();
            break;
        }
        default:
            emit reject();
        }
    }
}

void Combiner::openNextFile()
{
    if (unixNaming) {
        if (readURL.isEmpty())
            readURL = baseURL;
        else {
            QString name = readURL.fileName();
            int pos = name.length() - 1;
            QChar ch;

            do {
                ch = name.at(pos).toLatin1() + 1;
                if (ch == QChar('Z' + 1))
                    ch = 'A';
                if (ch == QChar('z' + 1))
                    ch = 'a';
                name[ pos ] = ch;
                pos--;
            } while (pos >= 0 && ch.toUpper() == QChar('A'));
            readURL = readURL.adjusted(QUrl::RemoveFilename);
            readURL.setPath(readURL.path() + name);
        }
    } else {
        QString index("%1");        /* determining the filename */
        index = index.arg(fileCounter++).rightJustified(3, '0');
        readURL = baseURL.adjusted(QUrl::RemoveFilename);
        readURL.setPath(readURL.path() + baseURL.fileName() + '.' + index);
    }

    /* creating a read job */
    combineReadJob = KIO::get(readURL, KIO::NoReload, KIO::HideProgressInfo);

    connect(combineReadJob, SIGNAL(data(KIO::Job*,QByteArray)),
            this, SLOT(combineDataReceived(KIO::Job*,QByteArray)));
    connect(combineReadJob, SIGNAL(result(KJob*)),
            this, SLOT(combineReceiveFinished(KJob*)));
    if (hasValidSplitFile)
        connect(combineReadJob, SIGNAL(percent(KJob*,ulong)),
                this, SLOT(combineWritePercent(KJob*,ulong)));

}

void Combiner::combineDataReceived(KIO::Job *, const QByteArray &byteArray)
{
    if (byteArray.size() == 0)
        return;

    crcContext->update((unsigned char *)byteArray.data(), byteArray.size());
    transferArray = QByteArray(byteArray.data(), byteArray.length());

    receivedSize += byteArray.size();

    if (combineWriteJob == 0) {
        combineWriteJob = KIO::put(writeURL, permissions, KIO::HideProgressInfo | KIO::Overwrite);

        connect(combineWriteJob, SIGNAL(dataReq(KIO::Job*,QByteArray&)),
                this, SLOT(combineDataSend(KIO::Job*,QByteArray&)));
        connect(combineWriteJob, SIGNAL(result(KJob*)),
                this, SLOT(combineSendFinished(KJob*)));
    }

     // continue writing and suspend read job until received data is handed over to the write job
    combineReadJob->suspend();
    combineWriteJob->resume();
}

void Combiner::combineReceiveFinished(KJob *job)
{
    combineReadJob = 0;   /* KIO automatically deletes the object after Finished signal */
    if (job->error()) {
        if (job->error() == KIO::ERR_DOES_NOT_EXIST) {
            if (fileCounter == 1) { // .000 file doesn't exist but .001 is still a valid first file
                firstFileIs000 = false;
                openNextFile();
            }
            else if (!firstFileIs000 && fileCounter == 2) { // neither .000 nor .001 exist
                combineAbortJobs();
                KMessageBox::error(0, i18n("Cannot open the first split file of %1.",
                                           baseURL.toDisplayString(QUrl::PreferLocalFile)));
                emit reject();
            } else { // we've received the last file
                // write out the remaining part of the file
                combineWriteJob->resume();

                if (hasValidSplitFile) {
                    QString crcResult = QString("%1").arg(crcContext->result(), 0, 16).toUpper().trimmed()
                                        .rightJustified(8, '0');

                    if (receivedSize != expectedSize)
                        error = i18n("Incorrect filesize, the file might have been corrupted.");
                    else if (crcResult != expectedCrcSum.toUpper().trimmed())
                        error = i18n("Incorrect CRC checksum, the file might have been corrupted.");
                }
            }
        } else {
            combineAbortJobs();
            static_cast<KIO::Job*>(job)->uiDelegate()->showErrorMessage();
            emit reject();
        }
    } else
        openNextFile();
}

void Combiner::combineDataSend(KIO::Job *, QByteArray &byteArray)
{
    byteArray = transferArray;
    transferArray = QByteArray();

    if (combineReadJob) {
        // continue reading and suspend write job until data is available
        combineReadJob->resume();
        combineWriteJob->suspend();
    }
}

void Combiner::combineSendFinished(KJob *job)
{
    combineWriteJob = 0;  /* KIO automatically deletes the object after Finished signal */

    if (job->error()) {   /* any error occurred? */
        combineAbortJobs();
        static_cast<KIO::Job*>(job)->uiDelegate()->showErrorMessage();
        emit reject();
    } else if (!error.isEmpty()) {  /* was any error message at reading ? */
        combineAbortJobs();             /* we cannot write out it in combineReceiveFinished */
        KMessageBox::error(0, error);   /* because emit accept closes it in this function */
        emit reject();
    } else
        emit accept();
}

void Combiner::combineAbortJobs()
{
    if (statJob)
        statJob->kill(KJob::Quietly);
    if (combineReadJob)
        combineReadJob->kill(KJob::Quietly);
    if (combineWriteJob)
        combineWriteJob->kill(KJob::Quietly);

    statJob = combineReadJob = combineWriteJob = 0;
}

void Combiner::combineWritePercent(KJob *, unsigned long)
{
    int percent = (int)((((double)receivedSize / expectedSize) * 100.) + 0.5);
    setValue(percent);
}


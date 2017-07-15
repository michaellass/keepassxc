/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdlib>
#include <stdio.h>

#include "Remove.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

#include "cli/Utils.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Tools.h"

Remove::Remove()
{
    this->name = QString("rm");
    this->description = QObject::tr("Remove an entry from the database.");
}

Remove::~Remove()
{
}

int Remove::execute(int argc, char** argv)
{
    QStringList arguments;
    // Skipping the first argument (keepassxc).
    for (int i = 1; i < argc; ++i) {
        arguments << QString(argv[i]);
    }

    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(this->description);
    parser.addPositionalArgument("database", QObject::tr("Path of the database."));
    parser.addPositionalArgument("entry", QObject::tr("Path of the entry to remove."));
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 2) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli rm");
        return EXIT_FAILURE;
    }

    Database* db = Database::unlockFromStdin(args.at(0));
    if (db == nullptr) {
        return EXIT_FAILURE;
    }

    return this->removeEntry(db, args.at(0), args.at(1));
}

int Remove::removeEntry(Database* database, QString databasePath, QString entryPath)
{

    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);
    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        qCritical("Entry %s not found.", qPrintable(entryPath));
        return EXIT_FAILURE;
    }

    QString entryTitle = entry->title();
    if (Tools::hasChild(database->metadata()->recycleBin(), entry) || !database->metadata()->recycleBinEnabled()) {
        if (!Utils::askYesNoQuestion("You are about to remove entry " + entryTitle + " permanently.", false, true)) {
            return EXIT_FAILURE;
        }
        delete entry;
    } else {
        database->recycleEntry(entry);
    };

    QString errorMessage = database->saveToFile(databasePath);
    if (!errorMessage.isEmpty()) {
        qCritical("Unable to save database to file : %s", qPrintable(errorMessage));
        return EXIT_FAILURE;
    }
    outputTextStream << "Successfully removed entry " << entryTitle << "." << endl;

    return EXIT_SUCCESS;
}

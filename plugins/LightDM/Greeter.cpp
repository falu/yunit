/*
 * Copyright (C) 2013-2016 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Greeter.h"
#include "GreeterPrivate.h"
#include <libintl.h>

static Greeter *singleton = nullptr;

GreeterPrivate::GreeterPrivate(Greeter* parent)
  : m_greeter(new QLightDM::Greeter(parent)),
    m_active(false),
    wasPrompted(false),
    promptless(false),
    q_ptr(parent)
{
}

Greeter::Greeter(QObject* parent)
  : QObject(parent),
    d_ptr(new GreeterPrivate(this))
{
    Q_D(Greeter);

    connect(d->m_greeter, &QLightDM::Greeter::showMessage,
            this, &Greeter::showMessageFilter);
    connect(d->m_greeter, &QLightDM::Greeter::showPrompt,
            this, &Greeter::showPromptFilter);
    connect(d->m_greeter, &QLightDM::Greeter::authenticationComplete,
            this, &Greeter::authenticationCompleteFilter);

    d->m_greeter->connectSync();
}

Greeter::~Greeter()
{
    singleton = nullptr;
}

Greeter *Greeter::instance()
{
    if (!singleton) {
        singleton = new Greeter;
    }
    return singleton;
}

bool Greeter::isActive() const
{
    Q_D(const Greeter);
    return d->m_active;
}

void Greeter::setIsActive(bool active)
{
    Q_D(Greeter);
    if (d->m_active != active) {
        d->m_active = active;
        Q_EMIT isActiveChanged();
    }
}

bool Greeter::isAuthenticated() const
{
    Q_D(const Greeter);
    return d->m_greeter->isAuthenticated();
}

QString Greeter::authenticationUser() const
{
    Q_D(const Greeter);
    return d->m_greeter->authenticationUser();
}

QString Greeter::defaultSessionHint() const
{
    Q_D(const Greeter);
    return d->m_greeter->defaultSessionHint();
}

bool Greeter::promptless() const
{
    Q_D(const Greeter);
    return d->promptless;
}

QString Greeter::selectUser() const
{
    Q_D(const Greeter);
    if (hasGuestAccount() && d->m_greeter->selectGuestHint()) {
        return QStringLiteral("*guest");
    } else {
        return d->m_greeter->selectUserHint();
    }
}

bool Greeter::hasGuestAccount() const
{
    Q_D(const Greeter);
    return d->m_greeter->hasGuestAccountHint();
}

bool Greeter::showManualLoginHint() const
{
    Q_D(const Greeter);
    return d->m_greeter->showManualLoginHint();
}

bool Greeter::hideUsersHint() const
{
    Q_D(const Greeter);
    return d->m_greeter->hideUsersHint();
}

void Greeter::authenticate(const QString &username)
{
    Q_D(Greeter);
    d->wasPrompted = false;
    if (d->promptless) {
        d->promptless = false;
        Q_EMIT promptlessChanged();
    }

    if (username == QStringLiteral("*guest")) {
        d->m_greeter->authenticateAsGuest();
    } else if (username == QStringLiteral("*other")) {
        d->m_greeter->authenticate(nullptr);
    } else {
        d->m_greeter->authenticate(username);
    }

    Q_EMIT isAuthenticatedChanged();
    Q_EMIT authenticationUserChanged();
}

void Greeter::respond(const QString &response)
{
    Q_D(Greeter);
    d->m_greeter->respond(response);
    Q_EMIT authenticationUserChanged(); // may have changed in liblightdm
}

bool Greeter::startSessionSync(const QString &session)
{
    Q_D(Greeter);
    return d->m_greeter->startSessionSync(session);
}

void Greeter::showPromptFilter(const QString &text, QLightDM::Greeter::PromptType type)
{
    Q_D(Greeter);
    d->wasPrompted = true;

    bool isDefaultPrompt = (text == dgettext("Linux-PAM", "Password: "));

    // Strip prompt of any colons at the end
    QString trimmedText = text.trimmed();
    if (trimmedText.endsWith(':') || trimmedText.endsWith(QStringLiteral("："))) {
        trimmedText.chop(1);
    }

    if (trimmedText == "login") {
        // 'login' is provided untranslated by LightDM when asking for a manual
        // login username.
        trimmedText = gettext("Username");
    }

    Q_EMIT showPrompt(trimmedText, type == QLightDM::Greeter::PromptTypeSecret, isDefaultPrompt);
}

void Greeter::showMessageFilter(const QString &text, QLightDM::Greeter::MessageType type)
{
    Q_EMIT showMessage(text, type == QLightDM::Greeter::MessageTypeError);
}

void Greeter::authenticationCompleteFilter()
{
    Q_D(Greeter);
    if (!d->wasPrompted) {
        d->promptless = true;
        Q_EMIT promptlessChanged();
    }

    Q_EMIT isAuthenticatedChanged();
    Q_EMIT authenticationComplete();
}

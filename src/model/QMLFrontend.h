/*
 *   This is part of the FaceLift project
 *   Copyright (C) 2017 Pelagicore AB
 *   SPDX-License-Identifier: LGPL-2.1
 *   This file is subject to the terms of the LGPL 2.1 license.
 *   Please see the LICENSE file for details.
 */

#pragma once

#include <QObject>
#include <QDebug>
#include <QQmlEngine>
#include <QtQml>
#include <functional>
#include <QQmlParserStatus>

#include "Model.h"

namespace facelift {

/*!
 * This is the base class which all QML frontends extend
 */
class QMLFrontendBase : public QObject, public QQmlParserStatus
{
    Q_OBJECT

public:
    QMLFrontendBase(QObject *parent = nullptr) :
        QObject(parent)
    {
    }

    Q_PROPERTY(QObject * provider READ provider CONSTANT)
    virtual InterfaceBase * provider() {
        Q_ASSERT(m_provider != nullptr);
        qWarning() << "Accessing private provider implementation object";
        return m_provider;
    }

    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    bool ready() const
    {
        return m_provider->ready();
    }

    Q_SIGNAL void readyChanged();

    Q_PROPERTY(QString implementationID READ implementationID CONSTANT)
    virtual const QString &implementationID() {
        static QString id = "";
        return id;
    }

    /**
     *
     */
    template<typename QMLFrontendType, typename InterfaceType>
    void synchronizeInterfaceProperty(QPointer<QMLFrontendType> &p, InterfaceType *i)
    {
        if (p.isNull() || (p->m_provider.data() != i)) {
            if (i != nullptr) {
                p = new QMLFrontendType(i);
                p->init(*i);
                p->setProvider(i);
            } else {
                p = nullptr;
            }
        }
    }

    void setProvider(InterfaceBase *provider)
    {
        m_provider = provider;
        connect(provider, &InterfaceBase::readyChanged, this, &QMLFrontendBase::readyChanged);
    }

    void classBegin() override
    {
    }

    void componentComplete() override
    {
        m_provider->componentCompleted();
    }

private:
    InterfaceBase *m_provider = nullptr;

};

/*!
 * This is the class which is registered when calling registerQmlComponent()
 * It is an actual instance of the QMLFrontendType and wraps an instance of the provider
 */
template<typename ProviderType>
class TQMLFrontend : public ProviderType::QMLFrontendType
{

public:
    TQMLFrontend(QObject *parent = nullptr) :
        ProviderType::QMLFrontendType(parent)
    {
        this->init(m_provider);
        this->setProvider(&m_provider);
    }

    virtual ~TQMLFrontend()
    {
    }

    virtual const QString &implementationID()
    {
        return m_provider.implementationID();
    }

    void componentComplete() override
    {
        m_provider.componentCompleted();  // notify anyone interested that we are ready (such as an IPC attached property)
    }

private:
    ProviderType m_provider;

};


template<typename Type>
QObject *singletonGetter(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine);
    Q_UNUSED(engine);
    auto obj = new Type();
    obj->componentComplete();
    qDebug() << "Singleton created" << obj;
    return obj;
}


/*!
 * Register the given interface QML implementation as a creatable QML component.
 * By default, the component is registered under the same name as defined in the QFace definition.
 */
template<typename ProviderType>
int registerQmlComponent(const char *uri, const char *name = ProviderType::QMLFrontendType::INTERFACE_NAME, int majorVersion =
        ProviderType::VERSION_MAJOR,
        int minorVersion = ProviderType::VERSION_MINOR,
        typename std::enable_if<std::is_base_of<facelift::InterfaceBase, ProviderType>::value>::type * = nullptr)
{
    ProviderType::registerTypes(uri);
    return ::qmlRegisterType<TQMLFrontend<ProviderType> >(uri, majorVersion, minorVersion, name);
}


/*!
 * Register the given interface QML implementation as a creatable QML component.
 * By default, the component is registered under the same name as defined in the QFace definition.
 */
template<typename ProviderType>
int registerSingletonQmlComponent(const char *uri, const char *name = ProviderType::QMLFrontendType::INTERFACE_NAME,
        int majorVersion = ProviderType::VERSION_MAJOR,
        int minorVersion = ProviderType::VERSION_MINOR,
        typename std::enable_if<std::is_base_of<facelift::InterfaceBase, ProviderType>::value>::type * = nullptr)
{
    ProviderType::registerTypes(uri);
    typedef TQMLFrontend<ProviderType> QMLType;
    return ::qmlRegisterSingletonType<QMLType>(uri, majorVersion, minorVersion, name, &singletonGetter<QMLType>);
}


class ModelListModelBase : public QAbstractListModel
{
    Q_OBJECT

public:
    typedef size_t (QObject::*SizeGetterFunction)();

    void notifyModelChanged()
    {
        auto previousRowCount = m_rowCount;
        auto newRowCount = (m_provider->*m_sizeGetter)();

        if (newRowCount > previousRowCount) {
            // And a beginInsertRows() & endInsertRows() for the new items
            beginInsertRows(QModelIndex(), previousRowCount, newRowCount - 1);
            syncWithProvider();
            endInsertRows();
        } else {
            // And a beginInsertRows() & endInsertRows() for the new items
            beginRemoveRows(QModelIndex(), newRowCount, previousRowCount - 1);
            syncWithProvider();
            endRemoveRows();
        }

        // Trigger "dataChanged()" for the items which were existing previously
        QModelIndex previousTopLeft = index(0, 0);
        QModelIndex previousBottomRight = index(m_rowCount - 1, 0);
        dataChanged(previousTopLeft, previousBottomRight);

    }

    void syncWithProvider()
    {
        m_rowCount = (m_provider->*m_sizeGetter)();
    }

protected:
    size_t m_rowCount = 0;
    SizeGetterFunction m_sizeGetter;
    QObject *m_provider;

};

template<typename ElementType>
class ModelListModel : public ModelListModelBase
{
public:
    typedef ElementType (QObject::*ElementGetterFunction)(size_t);

    ModelListModel()
    {
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[1000] = "modelData";
        return roles;
        //        return ElementType::roleNames_(ElementType::FIELD_NAMES);
    }

    int rowCount(const QModelIndex &index) const override
    {
        Q_UNUSED(index);
        return m_rowCount;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        Q_UNUSED(role);
        auto element = (m_provider->*m_elementGetter)(index.row());
        return QVariant::fromValue(element);
    }

    template<typename ProviderType>
    void init(QObject *ownerObject, void (ProviderType::*changeSignal)(), size_t (ProviderType::*sizeGetter)(),
            ElementType (ProviderType::*elementGetter)(size_t))
    {
        Q_UNUSED(changeSignal);
        m_provider = ownerObject;
        m_sizeGetter = (SizeGetterFunction) sizeGetter;
        m_elementGetter = (ElementGetterFunction) elementGetter;
        syncWithProvider();
    }

private:
    ElementGetterFunction m_elementGetter = nullptr;

};

}

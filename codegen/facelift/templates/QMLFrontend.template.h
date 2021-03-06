
{% set class = '{0}'.format(interface) %}
{% set comma = joiner(",") %}

/****************************************************************************

** This is an auto-generated file.
** Do not edit! All changes made to it will be lost.
****************************************************************************/

#pragma once

#include <QtCore>

#include "QMLFrontend.h"

#include "{{class}}.h"

// Dependencies
{% for property in interface.properties %}
{{property|requiredInclude}}
{{property|requiredQMLInclude}}
{% endfor %}

{% for operation in interface.operations %}
{% for parameter in operation.parameters %}
{{parameter|requiredInclude}}
{% endfor %}
{% endfor %}

{% for event in interface.signals %}
{% for parameter in event.parameters %}
{{parameter|requiredInclude}}
{% endfor %}
{% endfor %}


{{module|namespaceOpen}}


/**
* \class {{class}}QMLFrontend
* \ingroup {{interface.module.name|toValidId}}
* \inqmlmodule {{interface.module.name}}
*/

/**
 * This is the class registered as a QML component for the {{interface}} interface
 */
class {{class}}QMLFrontend : public facelift::QMLFrontendBase {

    Q_OBJECT

public:

    static constexpr const char* INTERFACE_NAME = "{{interface}}";

    {{class}}QMLFrontend(QObject* parent = nullptr) :
    	facelift::QMLFrontendBase(parent) {
    }

    void init({{class}}& provider) {
        m_provider = &provider;
        {% for property in interface.properties %}
        connect(m_provider, &{{class}}::{{property.name}}Changed, this, &{{class}}QMLFrontend::{{property.name}}Changed);

        {% if property.type.is_model -%}
        m_{{property}}Model.init(m_provider, &{{class}}::{{property}}Changed, &{{class}}::{{property.name}}Size, &{{class}}::{{property.name}}ElementAt);
        connect(m_provider, &{{class}}::{{property}}Changed, &m_{{property}}Model, &facelift::ModelListModel<{{property|nestedType|fullyQualifiedCppName}}>::notifyModelChanged);
        {% endif %}

        {% endfor %}

        {% for event in interface.signals %}
        connect(m_provider, &{{class}}::{{event.name}}, this, &{{class}}QMLFrontend::{{event.name}});
        {% endfor %}

    }

    {% for property in interface.properties %}

    {{property.comment}}

    {% if property.type.is_model -%}

    Q_PROPERTY(QObject* {{property}} READ {{property}} NOTIFY {{property.name}}Changed)
    QObject* {{property}}() {
    	return &m_{{property}}Model;
    }

    facelift::ModelListModel<{{property|nestedType|fullyQualifiedCppName}}> m_{{property}}Model;

    {% elif property.type.is_list -%}
    Q_PROPERTY(QList<QVariant> {{property}} READ {{property}} NOTIFY {{property.name}}Changed)   // Exposing QList<ActualType> to QML does not seem to work
    QList<QVariant> {{property}}() const {
        return facelift::toQMLCompatibleType(m_provider->{{property}}());
    }


    {% elif property.type.is_interface %}

    Q_PROPERTY(QObject* {{property}} READ {{property}} NOTIFY {{property.name}}Changed)

    {{property|returnType}}QMLFrontend* {{property}}() {
    	synchronizeInterfaceProperty(m_{{property}}Frontend, m_provider->{{property}}());
        return m_{{property}}Frontend;
    }

    QPointer<{{property|returnType}}QMLFrontend> m_{{property}}Frontend;

    {% else %}

        {% set QmlType=property|returnType %}
        {% if property.type.is_enum %}
            {% set QmlType=QmlType + "Gadget::Type" %}
        {% endif %}

        {% if property.readonly %}
    Q_PROPERTY({{QmlType}} {{property}} READ {{property}} NOTIFY {{property.name}}Changed)
        {% else %}
    Q_PROPERTY({{QmlType}} {{property}} READ {{property}} WRITE set{{property}} NOTIFY {{property.name}}Changed)

    void set{{property}}(const {{property|returnType}}& newValue) {
//    	qDebug() << "Request to set property {{property}} to " << newValue;
    	Q_ASSERT(m_provider);
    	m_provider->set{{property}}(newValue);
    }
        {% endif %}

    virtual const {{property|returnType}}& {{property}}() const {
    	Q_ASSERT(m_provider);
//    	qDebug() << "Read property {{property}}. Value: " << m_provider->{{property}}() ;
        return m_provider->{{property}}();
    }
    {% endif %}

    Q_SIGNAL void {{property}}Changed();

    {% endfor %}

    {% for operation in interface.operations %}
    Q_INVOKABLE virtual {{operation|returnType}} {{operation}}(
        {% set comma = joiner(",") %}
        {% for parameter in operation.parameters %}
        {{ comma() }}

        {% set QmlType=parameter|returnType %}
        {% if parameter.type.is_enum %}
            {% set QmlType=QmlType + "Gadget::Type" %}
        {% endif %}

        {{QmlType}} {{parameter.name}}
        {% endfor %}
    ) {
    	Q_ASSERT(m_provider);
        return m_provider->{{operation}}(
                {% set comma = joiner(",") %}
                {% for parameter in operation.parameters %}
                {{ comma() }}
                {{parameter.name}}
                {% endfor %}
                );

    }
    {% endfor %}


    {% for event in interface.signals %}
    Q_SIGNAL void {{event}}(
        {% set comma = joiner(",") %}
        {% for parameter in event.parameters %}
        {{ comma() }}

        {% set QmlType=parameter|returnType %}
        {% if parameter.type.is_enum %}
            {% set QmlType=QmlType + "Gadget::Type" %}
        {% endif %}

        {{QmlType}} {{parameter.name}}
        {% endfor %}
    );
    {% endfor %}

    QPointer<{{class}}> m_provider;
};


{{module|namespaceClose}}


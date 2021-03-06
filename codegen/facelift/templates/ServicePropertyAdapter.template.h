{% set class = '{0}'.format(interface) %}
/****************************************************************************
** This is an auto-generated file.
** Do not edit! All changes made to it will be lost.
****************************************************************************/

#pragma once

#include "{{class}}.h"
#include "Property.h"

{{module|namespaceOpen}}

/**
 * A partial implementation of the service interface, using the Property helper class
 */
class {{class}}PropertyAdapter : public {{class}} {

public:

    {{class}}PropertyAdapter(QObject* parent = nullptr) : {{class}}(parent) {
        {% for property in interface.properties %}
          m_{{property.name}}.init("{{property.name}}", this, &{{class}}::{{property.name}}Changed);
        {% endfor %}
    }

    {% for property in interface.properties %}

    {% if property.type.is_model %}

    {{property|nestedType|fullyQualifiedCppName}} {{property.name}}ElementAt(size_t index) override {
        return m_{{property.name}}.elementAt(index);
    }

    size_t {{property.name}}Size() override {
    	return m_{{property.name}}.size();
    }

    facelift::ModelProperty<{{property|nestedType|fullyQualifiedCppName}}> m_{{property.name}};

    {% elif property.type.is_list %}

    const {{property|returnType}}& {{property}}() const override {
        return m_{{property.name}}.value();
    }

    facelift::ListProperty<{{property|nestedType|fullyQualifiedCppName}}> m_{{property.name}};

    {% elif property.type.is_interface -%}

    // Service property
    {{property|returnType}}* {{property}}() override {
    	return m_{{property.name}}.value();
    }

    facelift::ServiceProperty<{{property|returnType}}> m_{{property.name}};

 // TODO

    {% else %}

    const {{property|returnType}}& {{property}}() const override {
        return m_{{property.name}}.value();
    }
    facelift::Property<{{property|returnType}}> m_{{property.name}};
    {% endif %}

    {% endfor %}

};

{{module|namespaceClose}}




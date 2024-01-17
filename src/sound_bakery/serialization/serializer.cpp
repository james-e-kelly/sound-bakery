#include "serializer.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/system.h"

#include <rttr/type.h>

using namespace SB::Core::Serialization;

static const char* s_ObjectTypeKey = "ObjectType";
static const char* s_ObjectIDKey   = "ObjectID";

#pragma region YAML

struct YAMLBase
{
    YAMLBase(YAML::Emitter& emitter) : m_emitter(emitter) {}

protected:
    YAML::Emitter& m_emitter;
};

struct Doc : public YAMLBase
{
    Doc(YAML::Emitter& emitter) : YAMLBase(emitter)
    {
        emitter << YAML::BeginDoc;
    }

    ~Doc() { m_emitter << YAML::EndDoc; }
};

template <typename T>
struct KeyValue : YAMLBase
{
    KeyValue(YAML::Emitter& emitter, rttr::string_view key, T value)
        : YAMLBase(emitter)
    {
        if (key.size())
        {
            emitter << YAML::Key << key.data() << YAML::Value << value;
        }
        else
        {
            emitter << YAML::Value << value;
        }
    }
};

struct Map : YAMLBase
{
    Map(YAML::Emitter& emitter) : YAMLBase(emitter)
    {
        emitter << YAML::BeginMap;
    }

    ~Map() { m_emitter << YAML::EndMap; }
};

struct ChildMap : YAMLBase
{
    ChildMap(YAML::Emitter& emitter, rttr::string_view key) : YAMLBase(emitter)
    {
        emitter << YAML::Key << key.data() << YAML::Value << YAML::BeginMap;
    }

    ~ChildMap() { m_emitter << YAML::EndMap; }
};

struct ChildSequence : YAMLBase
{
    ChildSequence(YAML::Emitter& emitter, rttr::string_view key)
        : YAMLBase(emitter)
    {
        emitter << YAML::Key << key.data() << YAML::Value << YAML::BeginSeq;
    }

    ~ChildSequence() { m_emitter << YAML::EndSeq; }
};

#pragma endregion

void loadSimpleProperty(YAML::Node& node,
                        const rttr::type& propertyType,
                        rttr::property& property,
                        rttr::instance& instance)
{
    bool success = false;

    if (!node.IsDefined())
    {
        assert(success);
        return;
    }

    rttr::variant loadedVariant = node.Scalar();

    if (propertyType.is_wrapper())
    {
        success = loadedVariant.convert(propertyType.get_wrapped_type());

        if (!success)
        {
            return;
        }
    }

    if (loadedVariant.convert(propertyType))
    {
        assert(loadedVariant.is_valid());
        assert(loadedVariant.get_type() == propertyType);

        success = property.set_value(instance, loadedVariant);
        assert(success);
    }
    else if (propertyType == rttr::type::get<std::string_view>())
    {
        std::string string          = loadedVariant.convert<std::string>();
        std::string_view stringView = string;

        // assert(stringView.size());

        success = property.set_value(instance, stringView);
    }
    else if (propertyType == rttr::type::get<rttr::variant>())
    {
        success = property.set_value(instance, loadedVariant);
        assert(success);
    }
    else
    {
        assert(false);
    }

    // assert(success);
}

void loadProperty(YAML::Node& node,
                  rttr::property property,
                  rttr::instance instance)
{
    if (property.is_readonly())
    {
        return;
    }

    assert(instance.is_valid());

    const rttr::type propertyType        = property.get_type();
    const rttr::string_view propertyName = property.get_name();

    if (propertyType == rttr::type::get<rttr::variant>())
    {
        loadSimpleProperty(node, propertyType, property, instance);
    }
    else if (propertyType.is_arithmetic() || propertyType.is_wrapper() ||
             propertyType == rttr::type::get<std::string>() ||
             propertyType == rttr::type::get<std::string_view>())
    {
        loadSimpleProperty(node, propertyType, property, instance);
    }
    else if (propertyType.is_enumeration())
    {
        if (std::string loadedEnumString = node.as<std::string>();
            loadedEnumString.size())
        {
            if (rttr::enumeration enumeration = property.get_enumeration();
                enumeration.is_valid())
            {
                if (rttr::variant enumValue =
                        enumeration.name_to_value(loadedEnumString);
                    enumValue.is_valid())
                {
                    bool success = property.set_value(instance, enumValue);
                    rttr::type instanceType = instance.get_type();
                    rttr::type derivedType  = instance.get_derived_type();
                    assert(success);
                }
                else
                {
                    assert(false);
                }
            }
        }
    }
    else if (propertyType.is_associative_container())
    {
        if (node.IsSequence())
        {
            rttr::variant containerVariant = property.get_value(instance);
            rttr::variant_associative_view associativeView =
                containerVariant.create_associative_view();
            const rttr::type keyType   = associativeView.get_key_type();
            const rttr::type valueType = associativeView.is_key_only_type()
                                             ? associativeView.get_key_type()
                                             : associativeView.get_value_type();

            associativeView.clear();

            for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
            {
                rttr::variant key = it->as<std::string>();

                if (keyType.is_wrapper())
                {
                    key.convert(keyType.get_wrapped_type());
                }

                key.convert(keyType);

                assert(key.is_valid());

                if (associativeView.is_key_only_type())
                {
                    associativeView.insert(key);
                }
                else
                {
                    rttr::variant value = (++it)->as<std::string>();

                    if (valueType.is_wrapper())
                    {
                        value.convert(valueType.get_wrapped_type());
                    }

                    value.convert(valueType);

                    assert(value.is_valid());

                    associativeView.insert(key, value);
                }
            }

            property.set_value(instance, containerVariant);
        }
    }
    else if (propertyType.is_sequential_container())
    {
        if (node.IsSequence())
        {
            rttr::variant sequenceVariant = property.get_value(instance);
            rttr::variant_sequential_view sequentialView =
                sequenceVariant.create_sequential_view();
            const rttr::type valueType = sequentialView.get_value_type();

            const bool needToCreate = sequentialView.get_size() == 0;
            int index               = 0;

            for (YAML::Node seq : node)
            {
                if (seq.IsScalar())
                {
                    rttr::variant loadedVariant = seq.as<std::string>();

                    if (valueType.is_wrapper())
                    {
                        loadedVariant.convert(valueType.get_wrapped_type());
                    }

                    loadedVariant.convert(valueType);

                    if (loadedVariant.is_valid())
                    {
                        sequentialView.insert(
                            sequentialView.begin() +
                                static_cast<int>(sequentialView.get_size()),
                            loadedVariant);
                    }
                }
                else if (seq.IsSequence())
                {
                    for (const YAML::Node childSeq : seq)
                    {
                        if (childSeq.IsScalar())
                        {
                            rttr::variant loadedVariant =
                                childSeq.as<std::string>();

                            if (valueType.is_wrapper())
                            {
                                loadedVariant.convert(
                                    valueType.get_wrapped_type());
                            }

                            loadedVariant.convert(valueType);

                            if (loadedVariant.is_valid())
                            {
                                sequentialView.insert(
                                    sequentialView.begin() +
                                        static_cast<int>(
                                            sequentialView.get_size()),
                                    loadedVariant);
                            }
                        }
                    }
                }
                else if (seq.IsMap())
                {
                    const std::size_t currentIndex = index++;

                    assert(valueType.is_class());

                    if (currentIndex >= sequentialView.get_size() &&
                        !needToCreate)
                    {
                        continue;
                    }

                    rttr::variant variant =
                        needToCreate ? valueType.create()
                                     : sequentialView.get_value(currentIndex);

                    assert(variant.is_valid());

                    for (const rttr::property childProperty :
                         valueType.get_properties())
                    {
                        if (YAML::Node prop =
                                seq[childProperty.get_name().data()])
                        {
                            loadProperty(prop, childProperty, variant);
                        }
                    }

                    const bool convert = variant.convert(valueType);
                    assert(convert);

                    if (needToCreate)
                    {
                        const rttr::variant_sequential_view::const_iterator
                            iterator = sequentialView.insert(
                                sequentialView.begin() + currentIndex, variant);

                        assert(iterator != sequentialView.end());
                    }
                    else
                    {
                        const bool set =
                            sequentialView.set_value(currentIndex, variant);
                        assert(set);
                    }
                }
            }

            property.set_value(instance, sequenceVariant);
        }
    }
    else if (propertyType.is_class())
    {
        if (node.IsMap())
        {
            rttr::variant classVariant = property.get_value(instance);

            for (const rttr::property classProperty :
                 classVariant.get_type().get_properties())
            {
                YAML::Node classNode = node[classProperty.get_name().data()];
                loadProperty(classNode, classProperty, classVariant);
            }

            property.set_value(instance, classVariant);
        }
    }
    else
    {
        assert(false);  // ignore the assert, just want to see what we haven't
                        // accounted for
        loadSimpleProperty(node, propertyType, property, instance);
    }
}

// -----
//
// -----

void Serializer::saveObject(SB::Core::Object* object, YAML::Emitter& emitter)
{
    if (object != nullptr)
    {
        Doc doc(emitter);

        saveInstance(emitter, object);
    }
}

void Serializer::saveSystem(SB::Engine::System* system, YAML::Emitter& emitter)
{
    if (system != nullptr)
    {
        Doc doc(emitter);

        saveInstance(emitter, system);
    }
}

void Serializer::loadSystem(SB::Engine::System* system, YAML::Node& node)
{
    if (system != nullptr)
    {
        loadProperties(node, system, system->get_type().get_method("onLoaded"));
    }
}

rttr::instance SB::Core::Serialization::Serializer::createAndLoadObject(
    YAML::Node& node, std::optional<rttr::method> onLoadedMethod)
{
    const rttr::type type =
        rttr::type::get_by_name(node[s_ObjectTypeKey].as<std::string>());

    const rttr::instance created = type.create();

    assert(created);

    if (created)
    {
        if (created.get_derived_type().is_derived_from<SB::Core::Object>())
        {
            SB::Core::ObjectTracker::get()->trackObject(
                created.try_convert<SB::Core::Object>());
        }

        loadProperties(node, created, onLoadedMethod);
    }

    return created;
}

bool SB::Core::Serialization::Serializer::loadProperties(
    YAML::Node& node,
    rttr::instance instance,
    std::optional<rttr::method> onLoadedMethod)
{
    bool result = false;

    if (instance)
    {
        for (rttr::property property :
             instance.get_derived_type().get_properties())
        {
            if (!property.is_readonly())
            {
                if (YAML::Node propertyNode = node[property.get_name().data()])
                {
                    loadProperty(propertyNode, property, instance);
                }
            }
        }

        if (onLoadedMethod && onLoadedMethod.value().is_valid())
        {
            if (const rttr::method& method = onLoadedMethod.value())
            {
                if (method.get_parameter_infos().empty())
                {
                    method.invoke(instance);
                }
            }
        }
    }

    return result;
}

#pragma region Save

bool SB::Core::Serialization::Serializer::saveInstance(YAML::Emitter& emitter,
                                                       rttr::instance instance)
{
    bool result = false;

    if (emitter.good() && instance.is_valid())
    {
        Map map(emitter);

        KeyValue<const char*> objectTypeValue(
            emitter, s_ObjectTypeKey,
            instance.get_derived_type().get_name().data());

        result = true;

        for (const rttr::property& property :
             instance.get_derived_type().get_properties())
        {
            rttr::variant propertyValue = property.get_value(instance);

            assert(propertyValue.is_valid());

            result &= saveVariant(emitter, property.get_name(), propertyValue);
        }
    }

    return result;
}

bool Serializer::saveVariant(YAML::Emitter& emitter,
                             rttr::string_view name,
                             rttr::variant& variant)
{
    bool result = false;

    if (variant && emitter.good())
    {
        const rttr::type type = variant.get_type();

        if (type.is_arithmetic() || type.is_wrapper() ||
            type == rttr::type::get<std::string>() ||
            type == rttr::type::get<std::string_view>())
        {
            result = saveStringVariant(emitter, name, variant);
        }
        else if (type.is_enumeration())
        {
            result = saveEnumVariant(emitter, name, variant);
        }
        else if (type.is_associative_container())
        {
            result = saveAssociateContainerVariant(emitter, name, variant);
        }
        else if (type.is_sequential_container())
        {
            result = saveSequentialContainerVariant(emitter, name, variant);
        }
        else if (type.is_class())
        {
            result = saveClassVariant(emitter, name, variant);
        }
        else
        {
            assert(false);
        }
    }

    return result;
}

bool SB::Core::Serialization::Serializer::saveStringVariant(
    YAML::Emitter& emitter, rttr::string_view name, rttr::variant variant)
{
    bool result = false;

    if (emitter.good() && variant.is_valid())
    {
        if (variant.get_type().is_wrapper())
        {
            variant = variant.extract_wrapped_value();
        }

        std::string string;
        std::string_view stringView;

        const char* saveString = nullptr;

        if (variant.can_convert(rttr::type::get<std::string>()))
        {
            string = variant.to_string(&result);

            if (result && !string.empty())
            {
                saveString = string.c_str();
            }
        }
        else if (variant.can_convert(rttr::type::get<std::string_view>()))
        {
            stringView = variant.convert<std::string_view>(&result);

            if (result && !stringView.empty())
            {
                saveString = stringView.data();
            }
        }

        if (saveString)
        {
            KeyValue<const char*> keyValue(emitter, name, saveString);
        }
    }

    return result;
}

bool SB::Core::Serialization::Serializer::saveEnumVariant(
    YAML::Emitter& emitter, rttr::string_view name, rttr::variant variant)
{
    bool result = false;

    if (emitter.good() && name.size() && variant.is_valid())
    {
        const rttr::enumeration enumeration =
            variant.get_type().get_enumeration();
        const rttr::string_view enumValueName =
            enumeration.value_to_name(variant);

        if (!enumValueName.empty())
        {
            result = true;

            KeyValue<const char*> keyValue(emitter, name, enumValueName.data());
        }
    }

    return result;
}

bool SB::Core::Serialization::Serializer::saveAssociateContainerVariant(
    YAML::Emitter& emitter, rttr::string_view name, rttr::variant variant)
{
    bool result = false;

    if (emitter.good() && name.size() && variant.is_valid())
    {
        const rttr::variant_associative_view view =
            variant.create_associative_view();
        const rttr::type keyType   = view.get_key_type();
        const rttr::type valueType = view.is_key_only_type()
                                         ? view.get_key_type()
                                         : view.get_value_type();

        if (!view.is_empty() && valueType.is_valid())
        {
            ChildSequence childSeq(emitter, name);

            result = true;

            for (const rttr::variant& item : view)
            {
                const std::pair<rttr::variant, rttr::variant> valuePair =
                    item.convert<std::pair<rttr::variant, rttr::variant>>();

                rttr::variant key = valuePair.first.extract_wrapped_value();
                rttr::variant value =
                    view.is_key_only_type()
                        ? valuePair.first.extract_wrapped_value()
                        : valuePair.second.extract_wrapped_value();

                if (keyType.is_wrapper())
                {
                    key.convert(keyType.get_wrapped_type());
                }

                if (valueType.is_wrapper())
                {
                    value.convert(valueType.get_wrapped_type());
                }

                // For maps
                if (view.is_key_only_type() == false)
                {
                    result &= saveVariant(emitter, rttr::string_view(), key);
                }

                result &= saveVariant(emitter, rttr::string_view(), value);
            }
        }
    }

    return result;
}

bool SB::Core::Serialization::Serializer::saveSequentialContainerVariant(
    YAML::Emitter& emitter, rttr::string_view name, rttr::variant variant)
{
    bool result = false;

    if (emitter.good() && name.size() && variant.is_valid())
    {
        const rttr::variant_sequential_view view =
            variant.create_sequential_view();
        const rttr::type valueType = view.get_value_type();

        ChildSequence childSeq(emitter, name);

        if (!view.is_empty() && valueType.is_valid())
        {
            result = true;

            for (rttr::variant item : view)
            {
                if (valueType.is_wrapper())
                {
                    item.convert(valueType.get_wrapped_type());
                }

                item.convert(valueType);

                result &= saveVariant(emitter, rttr::string_view(), item);
            }
        }
    }

    return result;
}

bool SB::Core::Serialization::Serializer::saveClassVariant(
    YAML::Emitter& emitter, rttr::string_view name, rttr::variant variant)
{
    bool result = false;

    if (emitter.good() && variant.is_valid())
    {
        ChildMap childMap(emitter,
                          name.size() ? name : variant.get_type().get_name());

        result = true;

        for (const rttr::property& property :
             variant.get_type().get_properties())
        {
            rttr::variant propertyValue = property.get_value(variant);

            result &= saveVariant(emitter, property.get_name(), propertyValue);
        }
    }

    return result;
}

#pragma endregion
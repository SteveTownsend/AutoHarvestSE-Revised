/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#include "PrecompiledHeaders.h"

#include "Collections/Condition.h"
#include "Collections/CollectionFactory.h"

namespace shse
{

std::unique_ptr<CollectionFactory> m_instance;

CollectionFactory& CollectionFactory::Instance()
{
	if (!m_instance)
	{
		m_instance.reset(new CollectionFactory);
	}
	return *m_instance;
}

std::unique_ptr<PluginCondition> CollectionFactory::ParsePlugin(const nlohmann::json& pluginRule) const
{
	std::vector<std::string> plugins;
	plugins.reserve(pluginRule.size());
	std::transform(pluginRule.cbegin(), pluginRule.cend(), std::back_inserter(plugins),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<PluginCondition>(plugins);
}

std::unique_ptr<FormListCondition> CollectionFactory::ParseFormList(const nlohmann::json& formListRule) const
{
	std::vector<std::pair<std::string, std::string>> pluginFormLists;
	pluginFormLists.reserve(formListRule.size());
	std::transform(formListRule.cbegin(), formListRule.cend(), std::back_inserter(pluginFormLists),
		[&](const nlohmann::json& next) { return std::make_pair(next["listPlugin"], next["formID"]); });
	return std::make_unique<FormListCondition>(pluginFormLists);
}

std::unique_ptr<FormsCondition> CollectionFactory::ParseForms(const nlohmann::json& formsRule) const
{
	std::vector<std::pair<std::string, std::vector<std::string>>> forms;
	forms.reserve(formsRule.size());
	std::transform(formsRule.begin(), formsRule.end(), std::back_inserter(forms),
		[&](const nlohmann::json& next)
	{
		std::vector<std::string> formIDs;
		formIDs.reserve(next["form"].size());
		for (const std::string& form : next["form"])
		{
			formIDs.push_back(form);
		}
		return std::make_pair(next["plugin"].get<std::string>(), formIDs);
	});
	return std::make_unique<FormsCondition>(forms);
}

std::unique_ptr<KeywordCondition> CollectionFactory::ParseKeyword(const nlohmann::json& keywordRule) const
{
	std::vector<std::string> keywords;
	keywords.reserve(keywordRule.size());
	std::transform(keywordRule.cbegin(), keywordRule.cend(), std::back_inserter(keywords),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<KeywordCondition>(keywords);
}

std::unique_ptr<SignatureCondition> CollectionFactory::ParseSignature(const nlohmann::json& signatureRule) const
{
	std::vector<std::string> signatures;
	signatures.reserve(signatureRule.size());
	std::transform(signatureRule.cbegin(), signatureRule.cend(), std::back_inserter(signatures),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<SignatureCondition>(signatures);
}

std::unique_ptr<ScopeCondition> CollectionFactory::ParseScope(const nlohmann::json& scopeRule) const
{
	std::vector<std::string> scopes;
	scopes.reserve(scopeRule.size());
	std::transform(scopeRule.cbegin(), scopeRule.cend(), std::back_inserter(scopes),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<ScopeCondition>(scopes);
}

CollectionPolicy CollectionFactory::ParsePolicy(const nlohmann::json& policy) const
{
	return CollectionPolicy(ParseCollectibleHandling(policy["action"].get<std::string>()),
		policy["notify"].get<bool>(), policy["repeat"].get<bool>());
}

std::unique_ptr<ConditionTree> CollectionFactory::ParseFilter(const nlohmann::json& filter, const unsigned int depth) const
{
	ConditionTree::Operator op;
	if (filter["operator"].get<std::string>() == "AND")
	{
		op = ConditionTree::Operator::And;
	}
	else
	{
		op = ConditionTree::Operator::Or;
	}
	std::unique_ptr<ConditionTree> root(std::make_unique<ConditionTree>(op, depth));
	for (const auto& condition : filter["condition"].items())
	{
		if (condition.key() == "subFilter")
		{
			for (const auto& subFilter : condition.value())
			{
				root->AddCondition(ParseFilter(subFilter, depth + 1));
			}
		}
		else if (condition.key() == "plugin")
		{
			root->AddCondition(ParsePlugin(condition.value()));
		}
		else if (condition.key() == "formList")
		{
			root->AddCondition(ParseFormList(condition.value()));
		}
		else if (condition.key() == "forms")
		{
			root->AddCondition(ParseForms(condition.value()));
		}
		else if (condition.key() == "keyword")
		{
			root->AddCondition(ParseKeyword(condition.value()));
		}
		else if (condition.key() == std::string("signature"))
		{
			root->AddCondition(ParseSignature(condition.value()));
		}
		else if (condition.key() == std::string("scope"))
		{
			root->AddCondition(ParseScope(condition.value()));
		}
	}

	return root;
}

std::shared_ptr<Collection> CollectionFactory::ParseCollection(
	const CollectionGroup* owningGroup, const nlohmann::json& collection, const CollectionPolicy& defaultPolicy) const
{
	const auto policy(collection.find("policy"));
	const std::string name(collection["name"].get<std::string>());
	bool overridesPolicy(policy != collection.cend());
	DBG_VMESSAGE("Collection {}, overrides Policy = {}", name.c_str(), overridesPolicy ? "true" : "false");

	return std::make_shared<Collection>(owningGroup, name, collection["description"].get<std::string>(),
		policy != collection.cend() ? ParsePolicy(collection["policy"]) : defaultPolicy, overridesPolicy, ParseFilter(collection["rootFilter"], 0));
}

std::shared_ptr<CollectionGroup> CollectionFactory::ParseGroup(const nlohmann::json& group, const std::string& groupName) const
{
	return std::make_shared<CollectionGroup>(groupName, ParsePolicy(group["groupPolicy"]), group["useMCM"].get<bool>(), group["collections"]);
}

}

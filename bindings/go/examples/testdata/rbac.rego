package rbac

import data.rbac.rules

default allow = {"allow": false}

shadow_rules = result {
	result = {"allow": true}
}

# compute rules will compute "ALLOW" rule or "DENY" rule

#  Handle ALLOW rule.
compute_rules(rules) = result {
	rules.action == "ALLOW"
	r = match_policies(rules.policies)
	result = handle_matched_for_allow(r)
}

# Handle DENY rule
compute_rules(rules) = result {
	rules.action == "DENY"
	r = match_policies(rules.policies)
	result = handle_matched_for_deny(r)
}

# convert matched rules to final rules for allow
handle_matched_for_allow(r) = result {
	count(r.policies) > 0
	result = {
		"allow": true,
		"policy": r.policies,
	}
}

handle_matched_for_allow(r) = result {
	count(r.policies) == 0
	result = {
		"allow": false,
		"policy": {},
	}
}

# convert matched rules to final rules for deny
handle_matched_for_deny(r) = result {
	count(r.policies) == 0
	result = {
		"allow": true,
		"policy": {},
	}
}

handle_matched_for_deny(r) = result {
	count(r.policies) > 0
	result = {
		"allow": false,
		"policy": r.policies,
	}
}

# check if policies can be matched
match_policies(policies) = result {
	result = {"policies": {k |
		# check if one policy is matched
		p := policies[k]
		match_permissions(p.permissions)
		match_principals(p.principals)
	}}
}

# check if permission matches.
match_permissions(perms) {
	p := perms[_]
	match_perm(p)
}

match_perm(p) {
	check_rpc_service(p.rpcService)
}

# TODO: this is only for test, need to handle and_rules and or_rules
check_rpc_service(service) {
	name := service.serviceName[k]
	k == "prefixMatch"
	startswith(input.serviceName, name)
}

# check if principal matches.
match_principals(principals) {
	ids := principals[i].andIds.ids

	# for and ids, every id should be matched
	r := checkIds(ids)
	count(r) == count(ids)
}

checkIds(ids) = result {
	count(ids) > 0
	result = {k |
		id := ids[k]
		appName := id.appIdentity.appName
		v := appName[exactMatch]
		v == input.appName
	}
}

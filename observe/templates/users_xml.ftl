<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<users>
<#list users as user>
    <user>
        <email>${user.email}</email>
        <firstName>${user.first_name}</firstName>
        <lastName>${user.last_name}</lastName>
        <login>${user.login}</login>
        <permission>${user.permission}</permission>
    </user>
</#list>
</users>

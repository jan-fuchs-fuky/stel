<#include "header.ftl">

<h1>Observe/Users</h1>

<#if message??>
<p><b>${message}</b></p>
</#if>

<#assign input_login = " readonly">
<#assign input_password = " readonly">
<#assign input_other = " readonly">
<#assign select_permission = " disabled">
<#assign input_submit = " disabled">
<#assign input_update = " disabled">
<#assign input_delete = " disabled">
<#assign action = "/observe/users">

<#if update?? && update = true>
    <#assign input_submit_value = "Update">
<#else>
    <#if role = "admin">
        <#assign input_login = "">
    </#if>

    <#assign input_submit_value = "Append">
</#if>

<#if change_pwd_uuid??>
    <#assign input_password = "">
    <#assign input_submit_value = "Change Password">
    <#assign action = "/observe/users/${login}/${change_pwd_uuid}">
</#if>

<#if role = "admin">
    <#assign input_password = "">
    <#assign input_other = "">
    <#assign select_permission = "">
    <#assign input_submit = "">
    <#assign input_update = "">
    <#assign input_delete = "">
<#elseif role = "permission">
    <#assign select_permission = "">
    <#assign input_submit = "">
    <#assign input_update = "">
</#if>

<#if login = (append.login)!>
    <#assign input_password = "">
    <#assign input_other = "">
    <#assign input_submit = "">
    <#assign input_update = "">
</#if>

<form name="edit" action="${action}" method="POST">
    <table>
        <tr><td>Login:</td>
            <td><input type="text" name="login" size="32" value="${(append.login)!}"${input_login}></td></tr>
        <tr><td>Password:</td>
            <td><input type="password" name="password" size="32"${input_password}></td></tr>
        <tr><td>Confirm password:</td>
            <td><input type="password" name="confirm_password" size="32"${input_password}></td></tr>
        <tr><td>First Name:</td>
            <td><input type="text" name="first_name" size="32" value="${(append.first_name)!}"${input_other}></td></tr>
        <tr><td>Last Name:</td>
            <td><input type="text" name="last_name" size="32" value="${(append.last_name)!}"${input_other}></td></tr>
        <tr><td>E-mail:</td>
            <td><input type="text" name="email" size="32" value="${(append.email)!}"${input_other}></td></tr>
        <tr><td>Permission:</td>
            <td>

                <select name="permission"${select_permission}>
                    <option value="none"<#if (append.permission)! = "none"> selected</#if>>none</option>
                    <option value="read"<#if (append.permission)! = "read"> selected</#if>>read</option>
                    <option value="control"<#if (append.permission)! = "control"> selected</#if>>control</option>

                <#if role = "admin">
                    <option value="admin"<#if (append.permission)! = "admin"> selected</#if>>admin</option>
                </#if>

                </select>

            </td></tr>
        <tr><td colspan="2">

            <input type="submit" value="${input_submit_value}"${input_submit}>

        </td></tr>
    </table>
</form>

<#if users??>

    <table class="users">
        <tr>
            <th></th>
            <th></th>
            <th></th>
            <th>Login</th>
            <th>First Name</th>
            <th>Last Name</th>
            <th>E-mail</th>
            <th>Permission</th>
        </tr>
    
    <#list users as user>
        <tr>
            <td>
                <form class="action" name="edit_${user.login}" action="/observe/users" method="POST">
                    <input type="hidden" name="login" value="${user.login}">
                    <input type="hidden" name="first_name" value="${user.first_name}">
                    <input type="hidden" name="last_name" value="${user.last_name}">
                    <input type="hidden" name="email" value="${user.email}">
                    <input type="hidden" name="permission" value="${user.permission}">

                    <#if login != "admin" && user.login = "tcsuser">
                        <input type="submit" value="Update" disabled>
                    <#elseif login = user.login>
                        <input type="submit" value="Update">
                    <#else>
                        <input type="submit" value="Update"${input_update}>
                    </#if>

                </form>
            </td>
            <td>
                <#if login = "tcsuser">
                    <form class="action" name="change_password_${user.login}" action="/observe/users/${user.login}/change_password" method="GET">
                        <input type="submit" value="Change Password">
                    </form>
                </#if>
            </td>
            <td>
                <form class="action" name="delete_${user.login}" action="/observe/users/${user.login}" method="POST">
                    <input type="submit" value="Delete"${input_delete}>
                </form>
            </td>
            <td>${user.login}</td>
            <td>${user.first_name}</td>
            <td>${user.last_name}</td>
            <td>${user.email}</td>
            <td>${user.permission}</td>
        </tr>
    </#list>
    
    </table>

</#if>

<#include "footer.ftl">

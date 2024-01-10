package cz.cas.asu.stelweb.fuky.observe.server.resources;

import java.util.Date;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import javax.persistence.Id;

@Entity
@Table(name="users")
public class User {

    private String login;
    private String password;
    private String firstName;
    private String lastName;
    private String email;
    private String permission;
    private Integer change_pwd_uuid;
    private Date change_pwd_date;

    public User() {
    }

    public User(String login, String password) {
        this.login = login;
        this.password = password;
    }

    @Id
    @Column(name="login")
    public String getLogin() {
        return this.login;
    }

    public void setLogin(String login) {
        this.login = login;
    }

    @Column(name="password")
    public String getPassword() {
        return this.password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    @Column(name="first_name")
    public String getFirstName() {
        return this.firstName;
    }

    public void setFirstName(String firstName) {
        this.firstName = firstName;
    }

    @Column(name="last_name")
    public String getLastName() {
        return this.lastName;
    }

    public void setLastName(String lastName) {
        this.lastName = lastName;
    }

    @Column(name="email")
    public String getEmail() {
        return this.email;
    }

    public void setEmail(String email) {
        this.email = email;
    }

    @Column(name="permission")
    public String getPermission() {
        return this.permission;
    }

    public void setPermission(String permission) {
        this.permission = permission;
    }
    
    @Column(name="change_pwd_uuid")
    public Integer getChangePwdUUID() {
        return this.change_pwd_uuid;
    }

    public void setChangePwdUUID(Integer change_pwd_uuid) {
        this.change_pwd_uuid = change_pwd_uuid;
    }
    
    @Column(name="change_pwd_date")
    public Date getChangePwdDate() {
        return this.change_pwd_date;
    }

    public void setChangePwdDate(Date change_pwd_date) {
        this.change_pwd_date = change_pwd_date;
    }
}

package cz.cas.asu.stelweb.fuky.observe.server.resources;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Table;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;

import org.hibernate.annotations.GenericGenerator;

@Entity
@Table(name="addresses")
public class Addresses {

    private String address;

    public Addresses() {
    }

    public Addresses(String address) {
        this.address = address;
    }

    @Id
    @Column(name="address")
    public String getAddress() {
        return this.address;
    }

    public void setAddress(String address) {
        this.address = address;
    }
}

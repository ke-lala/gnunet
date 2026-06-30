# Build

You can compile the binary using:

```
$ go build ./cmd/gns-registrar
```

# Running

Edit the file ```gns-registrar.conf``` to fit your needs:

```
[gns-registrar]
production = false                      # Set if this is a production deployment. Currently unused.
base_url = http://localhost:11000       # The base URL your service is reachable under. 
base_url_gnunet = http://localhost:7776 # The base URL your GNUnet REST service is reachable under.
basic_auth_gnunet_enabled = true        # Does the GNUnet REST service require authentication. 
basic_auth_gnunet_username = jdoe       # Basic authentication username for GNUnet REST service. 
basic_auth_gnunet_password = secret     # Basic authentication password for GNUnet REST service. 
base_url_merchant = https://backend.demo.taler.net  # The Taler merchant REST API base URL. 
merchant_token = sandbox                            # The Taler merchant REST API authentication token. 
bind_to = localhost:11000                           # The IP:PORT to bind to. 
landing_template = web/templates/landing.html       # The landing page template file. 
name_template = web/templates/name.html             # The name information page template file. 
edit_template = web/templates/edit.html             # The registration management template file. 
buy_template = web/templates/buy.html               # The checkout page template file. 
suffix_hint = example.alt                           # The self-advertised suffix for the zone. 
payment_required_expiration = 1h                    # The time the user is given to complete the payment for a registration. 
relative_delegation_expiration = 24h                # For how long a name registration record is valid/cached in GNS. 
registration_duration_days = 5                      # For how many days is a name registered (or a registration extended). 
registration_cost = KUDOS:0.3                       # The cost to register a name for the duration of ```registration_expiration```. 
order_summary_template = "Registration of `${NAME}' at GNUnet FCFS registrar" # Template for the order summary displayed to the user. May contain the placeholder ```${NAME}``` for the registered name.
root_zone_name = test                               # The GNS zone name to use. 
valid_label_regex = ""                              # The regex to check to verify the validity of a name. Ignored if set to ```""```. 
valid_label_script = ""                             # The script to use to check to verify the validity of a name. Script will be executed with the label as the first command line parameter. Ignored if set to ```""```. 
```

Make sure your GNUnet node is running, including the rest service.
Configure the GNUnet rest service base URL and authentication.
In particular, also setup a GNS zone and configure the zone name.
Make sure your Taler merchant is running and configure the merchant REST service base URL and authentication.
Then, execute:

```
$ ./gns-registrar
```
# TODOs

   * Subscription management:
      
      * Refunds (requires unique registration token)
      * Modification of registered delegation value (requires unique registration token)

0 */2 * * * root if [ -e /usr/share/glite-lb-logger/lb_krb_ticket.sh ]; then /usr/share/glite-lb-logger/lb_krb_ticket.sh 2>&1 | logger -t glite-lb-logger-krb-ticket -p user.error; fi

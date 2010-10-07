//gsoap wscalc service name:       CalcService
//gsoap wscalc service type:       Calc
//gsoap wscalc service port:       https://localhost:19999/
//gsoap wscalc service namespace:  urn:calc:wscalc
//gsoap wscalc service documentation: Calculator service

//gsoap wscalc service method-documentation: add Adds two numbers
int wscalc__add(double a, double b, struct wscalc__addResponse {
double result; } * );

//gsoap wscalc service method-documentation: sub Subtracts two numbers
int wscalc__sub(double a, double b, struct wscalc__subResponse {
double result; } * );

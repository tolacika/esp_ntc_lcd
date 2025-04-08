#ifndef SRC_PORTAL_H_
#define SRC_PORTAL_H_

#define PORTAL_SRC "<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"<meta charset=\"UTF-8\">\n" \
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
"<title>ESP32 Config Portal</title>\n" \
"</head>\n" \
"<body>\n" \
"<h1>ESP32 Config Portal</h1>\n" \
"<h2>Connect to WiFi</h2>\n" \
"<p>Please enter your WiFi credentials:</p>\n" \
"<form action=\"/connect\" method=\"POST\">\n" \
"<label for=\"ssid\">SSID:</label>\n" \
"<input type=\"text\" id=\"ssid\" name=\"ssid\" required><br><br>\n" \
"<label for=\"password\">Password:</label>\n" \
"<input type=\"password\" id=\"password\" name=\"password\" required><br><br>\n" \
"<input type=\"submit\" value=\"Connect\">\n" \
"</form>\n" \
"</body>\n" \
"</html>\n"

#endif /* SRC_PORTAL_H_ */
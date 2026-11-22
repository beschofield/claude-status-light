#include "settings.h"

#include <Preferences.h>

#include "config.h"

Settings settings;

// Lowercase letters, digits and dashes only, so the name works as NAME.local.
static String hostSafe(String s) {
  String out;
  s.toLowerCase();
  for (char c : s) {
    if (isalnum((unsigned char)c)) out += c;
    else if ((c == '-' || c == ' ' || c == '_') && out.length() && !out.endsWith("-")) out += '-';
  }
  while (out.endsWith("-")) out.remove(out.length() - 1);
  return out.length() ? out.substring(0, 32) : String(DEFAULT_NAME);
}

void settingsLoad() {
  Preferences p;
  p.begin("light", true);
  settings.name = hostSafe(p.getString("name", DEFAULT_NAME));
  settings.hub = p.getString("hub", "");
  settings.token = p.getString("token", "");
  settings.channel = p.getString("channel", DEFAULT_CHANNEL);
  p.end();
  if (!settings.channel.length()) settings.channel = DEFAULT_CHANNEL;
}

void settingsSave() {
  settings.name = hostSafe(settings.name);
  settings.hub.trim();
  while (settings.hub.endsWith("/")) settings.hub.remove(settings.hub.length() - 1);
  settings.token.trim();
  settings.channel.trim();
  if (!settings.channel.length()) settings.channel = DEFAULT_CHANNEL;
  Preferences p;
  p.begin("light", false);
  p.putString("name", settings.name);
  p.putString("hub", settings.hub);
  p.putString("token", settings.token);
  p.putString("channel", settings.channel);
  p.end();
}

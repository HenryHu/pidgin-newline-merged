/*
 * Displays messages on a new line, below the nick
 * Copyright (C) 2004 Stu Tomlinson <stu@nosnilmot.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#define PURPLE_PLUGINS

#include <string.h>

#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <signals.h>
#include <util.h>
#include <version.h>
#include <glib.h>

#ifndef _
#define _(a) (a)
#define N_(a) (a)
#endif

struct ConvNode {
	PurpleConversation *key;
	int sending;
	struct ConvNode *next;
};

#define HASH_SIZE 97

struct ConvNode *hash[HASH_SIZE];

static int
hash_calc(PurpleConversation *conv) {
	return ((long long)conv) % HASH_SIZE;
}

static int
hash_find(PurpleConversation *conv, int sending) {
	int key = hash_calc(conv);
	struct ConvNode *p = hash[key];
	while (p != NULL) {
		if (p->key == conv) {
			int old_sending = p->sending;
			p->sending = sending;
			return old_sending;
		}
		p = p->next;
	}
	p = malloc(sizeof(struct ConvNode));
	p->key = conv;
	p->sending = sending;
	p->next = hash[key];
	hash[key] = p;
	return -1;
}

void
hash_delete(PurpleConversation *conv) {
	int key = hash_calc(conv);
	struct ConvNode *p = hash[key];
	struct ConvNode *last = NULL;
	while (p != NULL) {
		if (p->key == conv) {
			if (last == NULL) {
				hash[key] = p->next;
			} else {
				last->next = p->next;
			}
			free(p);
			return;
		}
		last = p;
		p = p->next;
	}
}

static gboolean
addnewline_msg_cb(PurpleAccount *account, char *sender, char **message,
					 PurpleConversation *conv, PurpleMessageFlags flags, void *data)
{
	if (((purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) &&
		 !purple_prefs_get_bool("/plugins/core/newline_merged/im")) ||
		((purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) &&
		 !purple_prefs_get_bool("/plugins/core/newline_merged/chat")))
		return FALSE;

	if (g_ascii_strncasecmp(*message, "/me ", strlen("/me "))) {
		int now_sending = flags & PURPLE_MESSAGE_SEND ? 1 : 0;
		fprintf(stderr, "who:  %s\n", sender);
		fprintf(stderr, "msg:  %s\n", *message);
		fprintf(stderr, "send: %d\n", now_sending);
		int last_sending = -1;
		if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
			last_sending = hash_find(conv, now_sending);
		}
		if (last_sending == now_sending) {
//			*flags |= PURPLE_MESSAGE_RAW;
		} else {
			char *tmp = g_strdup_printf("<br/>%s", *message);
			g_free(*message);
			*message = tmp;
		}
	}

	return FALSE;
}

static void
deleteconv_cb(PurpleConversation *conv, void *data) {
	hash_delete(conv);
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin) {
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	frame = purple_plugin_pref_frame_new();

	ppref = purple_plugin_pref_new_with_name_and_label(
			"/plugins/core/newline_merged/im", _("Add new line in IMs"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(
			"/plugins/core/newline_merged/chat", _("Add new line in Chats"));
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}


static gboolean
plugin_load(PurplePlugin *plugin)
{
	void *conversation = purple_conversations_get_handle();

	purple_signal_connect(conversation, "writing-im-msg",
						plugin, PURPLE_CALLBACK(addnewline_msg_cb), NULL);
	purple_signal_connect(conversation, "writing-chat-msg",
						plugin, PURPLE_CALLBACK(addnewline_msg_cb), NULL);
	purple_signal_connect(conversation, "deleting-conversation",
						plugin, PURPLE_CALLBACK(deleteconv_cb), NULL);

	return TRUE;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame,
	0,   /* page_num (Reserved) */
	NULL, /* frame (Reserved) */
	/* Padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,							/**< magic			*/
	PURPLE_MAJOR_VERSION,							/**< major version	*/
	PURPLE_MINOR_VERSION,							/**< minor version	*/
	PURPLE_PLUGIN_STANDARD,							/**< type			*/
	NULL,											/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,						/**< priority		*/

	"core-henryhu-newline-merged",					/**< id				*/
	N_("New Line Merged"),							/**< name			*/
	"0.1.0",										/**< version		*/
	N_("Prepends a newline to displayed message."),	/**< summary		*/
	N_("Prepends a newline to messages so that the "
	   "rest of the message appears below the "
	   "username in the conversation window."),		/**< description	*/
	"Stu Tomlinson <stu@nosnilmot.com>\n"
		"Henry Hu <henry.hu.sh@gmail.com>",			/**< author			*/
	"",												/**< homepage		*/

	plugin_load,									/**< load			*/
	NULL,											/**< unload			*/
	NULL,											/**< destroy		*/

	NULL,											/**< ui_info		*/
	NULL,											/**< extra_info		*/
	&prefs_info,									/**< prefs_info		*/
	NULL,											/**< actions		*/

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin) {
	memset(&hash, 0, sizeof(hash));
	purple_prefs_add_none("/plugins/core/newline_merged");
	purple_prefs_add_bool("/plugins/core/newline_merged/im", TRUE);
	purple_prefs_add_bool("/plugins/core/newline_merged/chat", TRUE);
}

PURPLE_INIT_PLUGIN(newlinemerged, init_plugin, info)

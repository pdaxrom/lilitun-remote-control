// generated by Fast Light User Interface Designer (fluid) version 1,0400

#include "app.h"
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <simple-connection/tcp.h>
#include <FL/fl_ask.H>
#include <FL/Fl_PNG_Image.H>
#include "projector.h"
#include "../Common/protos.h"
static Fl_PNG_Image *logo = NULL; 
static Fl_PNG_Image *logo_gray = NULL; 
extern const char *json_requestType;
extern const char *json_appServerUrl;
extern const char *json_controlServerUrl;
extern const char *json_sessionId;
extern const char *server_url;
extern const char *server_privkey;
extern const char *server_cert;
extern const char *server_session_id;
static int user_port = -1; 
static char *user_password = NULL; 
static char *error_string = NULL; 
static pthread_t tid; 
static int is_started = 0; 
static Fl_Text_Buffer *log_buf = NULL; 
typedef struct { int err; char *cert; int ret; } ssl_verify_data;
static int accept_cert_error = false; 

static void set_error(char *str) {
  //Fl::lock();
  //status_string->value(str);
  //Fl::unlock();
  //Fl::flush();
  fprintf(stderr, "status: %s\n", str);
}

static void show_error() {
  fl_message_title("Error");
  fl_choice(error_string, fl_yes, NULL, NULL);
}

void error_for_exit(char *str) {
  if (error_string) {
      free(error_string);
  }
  error_string = strdup(str);
}

static void set_user_password(char *str) {
  fprintf(stderr, "User password is %s\n", str);
  if (user_password) {
      free(user_password);
  }
  user_password = strdup(str);
  
  Fl::lock();
  //set_logo(0);
  //copy_url_button->show();
  Fl::unlock();
  Fl::flush();
}

void set_user_connection(int port, int ipv6port) {
  fprintf(stderr, "User connection port is %d\n", port);
  user_port = port;
}

static void set_ssl_verify_cb(void *u) {
  ssl_verify_data *data = (ssl_verify_data *)u;
  
  fl_message_title("Remote host certificate error");
  data->ret = fl_choice("Error %d\n%s\nContinue?", "Yes", "No", "Yes(remember)", data->err, data->cert);
}

static int set_ssl_verify(int err, char *cert) {
  ssl_verify_data data = {
  	.err = err,
  	.cert = cert,
  	.ret = -1
  };
  
  if (accept_cert_error) {
  	return 1;
  }
  
  Fl::lock();
  Fl::awake(set_ssl_verify_cb, &data);
  Fl::unlock();
  
  while (data.ret == -1) {
  	sleep(1);
  }
  
  if (data.ret == 2) {
  	accept_cert_error = true;
  	data.ret = 0;
  }
  
  return (data.ret == 0) ? 1: 0;
}

static void * projector_thread(void *arg) {
  struct projector_t *projector = (struct projector_t *)arg;
  
  pthread_detach(pthread_self());
  
  while (is_started) {
  	int status = projector_connect(projector, server_url ? server_url : json_controlServerUrl, server_url ? NULL : json_appServerUrl, server_privkey, server_cert, server_url ? server_session_id : json_sessionId, &is_started);
  
  	if (status == STATUS_CONNECTION_ERROR) {
  	 	sleep(2);
  		 	continue;
  	}
  
  	break;
  }
  
  projector_finish(projector);
  
  Fl::lock();
  //set_logo(1);
  //copy_url_button->hide();
  share_button->label("Start remote control");
  share_button->clear();
  share_button->redraw();
  Fl::unlock();
  Fl::flush();
  
  return NULL;
}

int projector_start() {
  struct projector_t *projector = projector_init();
  if (!projector) {
      if (error_string) {
      	fl_message_title("Error");
      	fl_choice(error_string, "Close", NULL, NULL);
      	free(error_string);
      	exit(1);
      }
      return false;
  }
  
  projector->cb_error = set_error;
  projector->cb_user_password = set_user_password;
  projector->cb_user_connection = set_user_connection;
  projector->cb_ssl_verify = set_ssl_verify;
  
  is_started = true;
  
  share_button->label("Stop remote control");
  if (pthread_create(&tid, NULL, &projector_thread, projector) != 0) {
      is_started = false;
      share_button->label("Start remote control");
      return false;
  }
  
  return true;
}

static void projector_stop() {
  is_started = false;
}

Fl_Double_Window *win_main=(Fl_Double_Window *)0;

Fl_Light_Button *share_button=(Fl_Light_Button *)0;

static void cb_share_button(Fl_Light_Button* o, void*) {
  if (o->value() == 1) {
	fprintf(stderr, "Turn on\n");
	if (!projector_start()) {
		o->clear();
	}
} else {
	fprintf(stderr, "Turn off\n");
	o->set();
	projector_stop();
};
}

Fl_Text_Display *log_window=(Fl_Text_Display *)0;

Fl_Button *log_button=(Fl_Button *)0;

static void cb_log_button(Fl_Button*, void*) {
  if (log_window->visible()) {
	log_window->hide();
	win_main->resize(win_main->x(), win_main->y(), win_main->w(), 35);
} else {
	log_window->show();
	win_main->resize(win_main->x(), win_main->y(), win_main->w(), 225);
};
}

#include <FL/Fl_Image.H>
static const unsigned char idata_icons8[] =
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,49,0,0,0,208,0,0,0,254,0,0,0,255,0,0,0,
255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,252,0,0,0,137,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,208,0,0,0,226,0,0,0,137,0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,136,
0,0,0,136,0,0,0,255,0,0,0,255,0,0,0,143,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,253,0,
0,0,131,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,231,0,
0,0,255,0,0,0,143,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,128,0,0,0,135,0,0,0,255,0,0,0,143,0,
0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,255,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,255,0,0,0,128,0,0,0,0,0,0,0,135,0,0,0,255,0,0,0,143,0,0,0,1,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,128,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,253,0,0,0,131,0,0,0,0,0,
0,0,0,0,0,0,143,0,0,0,255,0,0,0,143,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,208,0,0,0,226,0,0,0,125,0,0,0,119,0,0,0,119,0,0,0,
227,0,0,0,255,0,0,0,138,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,255,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,49,0,0,0,207,0,0,0,253,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,252,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,
0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,159,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,
255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,
255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,159,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,254,0,0,0,196,0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,
136,0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,
136,0,0,0,136,0,0,0,196,0,0,0,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,255,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,109,0,0,0,109,0,0,0,0,0,0,0,92,0,
0,0,236,0,0,0,186,0,0,0,10,0,0,0,96,0,0,0,240,0,0,0,164,0,0,0,0,0,0,0,0,0,0,0,
128,0,0,0,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,
128,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,128,0,0,0,0,0,0,0,231,0,0,0,53,0,0,0,181,0,
0,0,101,0,0,0,232,0,0,0,54,0,0,0,92,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,255,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,128,0,0,0,0,0,0,0,0,
0,0,0,128,0,0,0,128,0,0,0,0,0,0,0,229,0,0,0,62,0,0,0,179,0,0,0,103,0,0,0,232,
0,0,0,56,0,0,0,218,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,255,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,109,0,0,0,
255,0,0,0,218,0,0,0,84,0,0,0,235,0,0,0,189,0,0,0,13,0,0,0,95,0,0,0,239,0,0,0,
164,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,254,0,0,0,187,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,
0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,
119,0,0,0,119,0,0,0,187,0,0,0,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,158,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,
255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,
255,0,0,0,255,0,0,0,157,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,253,0,0,0,131,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,131,0,0,0,253,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,208,0,0,0,226,0,0,0,125,0,0,
0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,0,0,119,0,
0,0,119,0,0,0,119,0,0,0,125,0,0,0,227,0,0,0,206,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,49,0,0,0,207,0,0,0,253,0,0,0,255,0,0,
0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255,0,
0,0,255,0,0,0,253,0,0,0,206,0,0,0,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static Fl_Image *image_icons8() {
  static Fl_Image *image = new Fl_RGB_Image(idata_icons8, 24, 24, 4, 0);
  return image;
}

Fl_Double_Window* init_gui() {
  { win_main = new Fl_Double_Window(250, 225, "LiliTun remote control");
    { share_button = new Fl_Light_Button(5, 5, 210, 25, "Start remote control");
      share_button->selection_color((Fl_Color)1);
      share_button->callback((Fl_Callback*)cb_share_button);
      share_button->align(Fl_Align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE));
      share_button->clear_visible_focus();
    } // Fl_Light_Button* share_button
    { log_window = new Fl_Text_Display(5, 35, 240, 185);
      log_window->hide();
      if (!log_buf) { log_buf = new Fl_Text_Buffer(); }
      log_window->buffer(log_buf);
    } // Fl_Text_Display* log_window
    { log_button = new Fl_Button(220, 5, 25, 25);
      log_button->image( image_icons8() );
      log_button->callback((Fl_Callback*)cb_log_button);
      log_button->align(Fl_Align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE));
      log_button->clear_visible_focus();
    } // Fl_Button* log_button
    Fl_PNG_Image *icon = new Fl_PNG_Image(NULL, logo_data, sizeof(logo_data));
    win_main->icon(icon);
    win_main->end();
  } // Fl_Double_Window* win_main
  return win_main;
}
unsigned char logo_data[6992] = /* data inlined from lilitun-remote-desktop-color.png */
{137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,100,0,0,0,100,8,6,0,0,
0,112,226,149,84,0,0,23,179,122,84,88,116,82,97,119,32,112,114,111,102,105,
108,101,32,116,121,112,101,32,101,120,105,102,0,0,120,218,205,156,91,118,35,43,
144,174,223,25,69,15,33,185,195,112,184,174,213,51,56,195,239,47,32,83,150,108,
201,101,91,251,225,148,203,150,149,32,50,238,241,71,64,90,141,255,247,191,83,
253,15,255,178,139,135,114,62,166,144,67,56,248,231,178,203,166,240,75,58,246,
191,188,126,234,195,173,159,235,95,112,231,152,126,188,174,110,3,134,75,150,87,
187,223,198,114,206,47,92,247,31,31,184,238,161,235,227,117,149,206,17,147,206,
133,244,109,225,245,207,202,157,229,247,126,79,36,215,205,190,174,79,66,84,30,
39,201,57,197,123,82,235,185,80,187,40,78,31,223,238,70,214,126,145,247,234,
225,66,68,74,221,115,35,107,204,176,218,30,235,103,218,20,88,249,214,182,240,
234,248,169,45,68,241,51,240,187,179,89,173,151,107,49,4,242,192,222,245,122,28,
247,2,122,16,242,245,155,250,44,253,219,111,159,132,111,202,121,221,126,146,101,
56,101,196,47,79,7,180,255,116,221,222,110,99,238,111,108,111,20,153,199,129,
150,140,253,194,206,249,61,103,79,115,142,205,93,113,1,137,134,211,162,150,176,
245,181,12,19,43,34,183,235,99,129,175,200,183,231,247,184,190,50,95,233,40,71,
67,229,253,104,71,229,171,233,172,13,90,153,74,59,221,117,209,83,143,245,218,
116,131,68,103,134,137,188,26,211,80,148,92,75,54,154,108,154,21,61,57,249,210,
211,68,155,109,183,9,93,54,51,148,181,92,54,55,90,244,186,111,94,247,107,58,113,
231,174,153,106,52,139,105,62,242,242,75,125,55,248,155,47,53,103,19,17,233,35,
221,100,5,93,70,236,26,50,68,115,242,147,89,40,68,207,83,111,126,9,248,250,58,
213,127,220,217,15,166,138,6,253,18,115,130,193,114,212,189,68,245,250,195,182,
236,210,179,101,158,231,117,187,144,86,177,159,11,32,34,238,237,33,70,91,52,112,
4,109,189,14,250,136,198,68,173,145,99,66,65,5,202,141,117,166,162,1,237,189,
233,16,105,156,181,193,168,104,146,145,123,243,153,168,215,92,227,77,48,114,153,
216,132,34,60,222,20,209,77,182,5,101,57,231,177,159,232,18,54,84,188,245,206,
123,31,124,244,73,249,236,75,176,193,5,31,66,136,65,130,92,137,54,186,232,99,
136,49,166,152,99,73,54,185,228,83,72,49,165,148,83,201,38,91,98,160,207,33,199,
156,114,206,165,24,85,184,81,97,173,194,252,194,149,106,170,173,174,250,26,106,
172,169,230,90,26,230,211,92,243,45,180,216,82,203,173,116,211,109,39,76,244,
208,99,79,61,247,50,180,26,68,138,225,134,31,97,196,145,70,30,101,98,107,211,78,
55,253,12,51,206,52,243,44,55,173,157,90,253,242,245,11,173,233,83,107,102,105,
74,230,197,155,214,184,170,98,188,150,208,18,78,188,232,12,141,25,167,209,120,
20,13,96,208,70,116,118,36,237,156,17,205,137,206,142,108,112,10,111,32,210,
139,110,84,215,162,49,84,232,134,54,126,234,155,238,62,52,247,35,189,41,159,126,
164,55,243,47,205,41,81,221,127,161,57,133,234,190,234,237,137,214,186,228,185,
182,52,182,189,80,100,122,88,188,143,241,145,138,50,169,72,82,43,239,190,254,
125,161,218,139,29,58,205,102,96,51,122,85,186,14,189,54,167,75,115,121,134,134,
84,16,20,170,168,166,139,109,230,48,170,33,19,244,218,125,211,196,66,77,170,
155,205,143,238,14,61,204,4,59,32,170,57,197,32,75,138,77,44,15,51,155,240,220,
114,205,242,58,83,45,118,14,22,153,73,71,228,112,196,57,51,134,48,101,84,71,22,
154,166,117,237,167,183,113,42,40,147,80,128,52,37,248,243,109,144,57,169,194,
12,18,70,242,206,142,195,203,245,34,209,109,184,222,214,50,205,51,61,182,106,
47,130,14,117,145,115,28,139,160,77,14,239,158,145,3,132,88,4,113,187,207,4,53,
117,17,180,224,198,90,98,147,116,28,155,40,150,188,200,90,68,113,125,145,197,
228,7,178,212,162,235,63,144,147,90,116,253,7,114,82,66,208,127,33,39,117,79,
208,59,114,82,55,253,189,41,39,117,211,223,155,114,82,23,65,239,202,73,125,38,
232,175,114,82,15,118,254,134,156,212,131,157,191,33,39,181,9,170,46,54,55,152,
236,139,27,220,221,48,41,37,235,98,182,163,197,48,67,213,46,148,30,124,245,195,
77,171,123,113,71,241,132,28,75,50,73,81,183,160,236,76,177,143,74,128,1,36,
150,89,114,29,250,16,97,155,222,117,157,163,47,5,64,82,54,117,248,252,114,134,
90,83,210,1,220,238,193,250,52,71,203,128,186,25,245,8,51,215,90,137,217,129,
80,198,79,59,170,207,75,252,71,134,85,64,31,194,114,1,38,91,72,96,200,165,145,
86,153,210,182,182,130,47,195,151,218,51,200,80,135,212,29,105,198,27,95,102,
155,72,222,128,25,66,240,169,20,75,198,241,182,68,83,141,15,67,165,90,205,52,
182,160,156,228,26,247,200,109,233,203,32,124,168,112,162,130,176,174,144,65,
209,148,94,140,84,45,55,75,5,126,74,90,215,85,156,162,206,16,161,38,52,50,234,
92,234,70,59,131,12,105,95,49,242,245,166,138,187,250,6,70,112,152,82,60,54,
155,88,205,156,96,231,209,83,227,163,24,16,166,213,210,40,124,22,104,128,73,158,
51,83,23,12,223,197,62,85,94,50,250,184,33,26,106,161,146,181,205,69,53,36,157,
100,107,64,180,230,195,24,87,227,66,214,221,205,147,115,167,206,169,194,58,140,
219,243,150,88,130,220,116,221,146,219,164,30,77,119,94,79,175,79,102,176,210,
69,19,111,107,176,124,90,97,157,137,107,127,22,183,29,101,113,163,60,62,183,
104,226,221,135,180,63,100,253,148,245,237,196,31,119,229,158,106,89,206,163,
188,79,105,11,198,187,228,45,112,225,179,196,31,217,87,167,208,159,40,249,65,
226,223,202,27,89,25,245,148,123,251,92,205,15,18,223,242,190,73,91,205,21,156,
146,217,226,45,249,43,247,155,249,155,204,221,163,158,47,137,171,197,204,150,
248,163,188,127,105,221,234,84,242,219,214,173,244,43,21,255,210,186,213,87,174,
255,102,221,74,204,251,159,214,237,193,186,196,53,168,41,125,196,56,134,109,32,
214,10,241,122,140,54,34,241,88,217,110,106,50,5,222,155,238,162,212,101,2,59,
248,247,92,8,101,26,40,78,189,69,40,207,206,135,58,237,96,45,162,55,1,206,148,
26,164,22,27,197,169,50,114,159,197,140,6,15,82,108,71,162,176,19,224,184,40,
10,66,24,137,33,122,73,54,43,105,189,152,161,206,41,169,197,206,157,215,212,99,
9,157,169,91,58,97,253,148,201,249,99,185,47,115,212,109,18,169,23,69,136,201,
153,177,220,120,167,200,158,151,254,132,245,87,99,179,232,118,172,133,106,109,
17,117,230,10,112,174,22,91,136,61,139,235,82,34,161,38,210,158,207,35,28,126,
189,57,214,171,47,221,162,93,138,219,176,44,223,119,213,183,175,1,165,173,137,
193,245,76,165,54,45,149,79,172,217,135,84,38,151,70,144,92,24,179,33,180,187,
130,85,251,150,116,167,62,11,36,78,49,80,211,171,66,61,121,156,216,34,162,212,
149,203,189,31,177,91,68,236,112,36,144,74,92,94,67,37,210,107,233,217,118,93,
142,110,72,70,163,199,105,182,175,170,168,81,106,12,103,226,11,29,33,182,169,55,
116,32,181,174,235,146,196,246,125,95,220,182,128,106,161,78,62,74,250,131,198,
99,167,203,16,177,109,32,135,209,177,139,82,220,160,130,44,99,197,137,107,61,
238,186,87,92,235,225,180,216,219,138,16,20,103,203,45,108,19,4,50,102,246,27,
27,21,220,173,117,108,56,83,118,237,172,140,154,197,236,72,236,29,225,78,210,
250,56,178,106,193,107,223,69,2,231,90,247,43,173,117,112,154,42,102,55,98,18,
32,211,241,13,102,123,241,186,92,150,21,225,61,83,188,63,1,119,88,42,185,141,
125,146,159,81,92,219,166,70,49,181,237,247,113,45,44,104,173,38,216,200,159,
129,90,189,22,47,62,224,161,58,246,56,118,112,195,10,89,246,96,162,211,204,28,
226,234,55,93,169,249,168,170,189,18,235,116,10,98,204,39,213,152,70,160,186,55,
45,37,45,6,128,41,161,32,2,5,65,78,186,150,35,186,209,167,93,150,157,240,21,23,
77,164,50,239,213,187,81,251,232,167,43,0,207,12,176,166,83,56,175,176,162,125,
133,183,34,228,32,243,162,53,165,52,24,205,121,213,59,246,154,50,142,97,40,158,
99,221,158,224,157,251,112,9,226,246,56,134,164,164,190,111,139,46,90,151,224,
124,55,162,30,134,32,119,153,26,165,62,174,70,230,40,128,200,45,187,102,197,27,
177,167,8,74,228,163,232,184,180,213,154,91,194,215,216,81,23,132,188,96,237,
243,41,183,25,248,76,111,175,66,141,250,73,172,249,73,168,81,63,137,53,63,9,53,
234,83,172,121,12,52,190,75,148,145,24,227,5,101,231,124,105,206,46,205,241,182,
232,67,218,32,32,54,75,138,144,58,30,151,42,182,118,201,82,195,105,31,153,68,
157,191,153,210,22,107,209,94,144,103,222,73,209,20,73,153,53,177,32,190,29,106,
27,202,96,23,136,18,183,139,37,99,30,122,49,162,131,30,233,12,143,37,196,54,
210,125,124,124,120,197,250,6,164,170,105,142,178,253,169,140,35,159,222,198,
253,36,54,198,205,166,246,139,77,188,79,156,3,202,60,40,152,224,226,96,108,232,
129,107,183,217,213,65,78,223,64,33,31,146,253,102,143,184,44,1,34,117,4,86,253,
197,140,15,139,25,98,6,204,80,151,148,14,41,142,0,2,199,49,69,175,4,45,152,233,
195,230,195,231,252,75,139,190,70,212,187,22,125,153,171,122,215,162,47,99,85,
239,90,244,101,208,234,93,139,190,12,90,189,107,209,151,65,171,119,45,250,50,
104,245,174,69,95,6,173,222,181,232,203,160,213,187,22,125,25,180,122,215,162,
47,131,86,239,90,244,101,208,234,93,139,190,12,90,189,107,209,215,152,122,215,
162,47,131,86,239,90,244,101,208,234,59,139,206,57,82,214,244,98,200,198,201,70,
120,182,90,103,126,177,32,149,212,124,180,114,167,132,217,244,157,251,101,169,
93,202,148,202,4,224,149,201,221,36,12,141,207,64,135,205,237,64,1,14,135,24,
153,170,39,64,121,68,66,82,155,236,58,44,69,37,171,125,90,236,44,125,45,152,102,
183,199,112,131,179,107,226,151,34,252,216,149,138,141,227,44,181,166,38,212,82,
171,72,97,3,210,2,12,90,43,213,85,189,42,201,103,139,45,164,40,117,203,2,102,59,
218,11,62,170,70,48,149,64,164,17,230,150,146,12,192,147,148,146,22,140,41,173,
147,15,46,142,83,40,115,247,217,164,183,37,76,168,121,187,237,94,221,236,166,
152,220,184,71,177,209,214,103,178,231,200,104,113,23,239,38,18,63,168,140,119,
203,70,170,74,165,197,119,4,69,45,94,249,68,207,11,22,94,183,253,36,189,50,5,
176,230,58,197,205,184,222,211,148,114,210,77,133,18,103,206,226,144,143,241,
229,49,188,108,150,95,41,64,247,234,136,217,33,86,151,194,209,59,198,92,157,119,
93,202,225,120,88,141,93,165,209,156,183,165,106,87,14,234,189,106,179,13,71,
168,3,196,231,50,117,89,47,68,152,66,240,153,84,144,187,151,32,198,29,243,174,
116,250,220,133,142,221,6,46,140,159,224,81,232,28,107,244,130,143,21,254,147,
142,5,156,205,173,127,144,110,165,206,238,130,201,191,4,167,61,162,230,35,128,
252,28,154,190,4,38,62,118,198,29,28,93,180,122,198,29,117,15,31,95,77,250,54,
56,133,45,8,245,13,124,60,167,60,6,38,108,104,133,166,101,40,119,97,73,61,27,
58,131,210,151,144,68,80,126,25,148,212,215,52,187,244,118,43,80,209,155,35,21,
174,13,178,85,76,28,213,133,177,138,9,107,142,17,74,224,198,58,100,165,139,55,
217,164,214,93,158,46,68,204,165,96,96,173,85,234,9,194,173,52,65,125,167,4,109,
169,30,20,185,157,178,52,123,91,227,17,171,110,195,230,50,13,150,221,0,236,41,
45,235,97,174,184,6,54,152,86,227,153,76,86,195,148,13,170,153,143,58,67,73,1,
137,215,220,169,82,76,234,85,90,5,20,61,150,218,118,69,115,245,77,37,22,19,52,
218,148,138,142,137,90,73,66,89,79,221,181,16,240,162,94,28,180,83,144,139,247,
235,153,212,115,122,210,30,149,193,115,232,56,246,224,106,221,215,126,124,84,
203,187,186,85,31,229,237,63,171,91,88,165,180,91,115,145,103,221,165,150,84,
117,163,219,67,22,178,20,190,59,151,241,255,34,245,183,148,170,147,84,55,169,3,
93,208,157,151,94,66,34,56,120,239,9,62,160,141,67,130,191,69,30,72,56,75,97,
44,189,161,99,85,249,41,236,144,19,135,10,171,147,128,30,125,106,68,111,111,
204,104,67,19,196,203,42,144,203,208,201,21,207,229,18,143,92,67,222,141,188,
118,240,126,134,99,54,231,182,148,84,47,235,40,72,115,241,220,152,248,24,207,
163,239,140,114,14,235,232,165,227,167,241,245,180,202,230,102,253,46,169,137,
32,74,38,32,108,135,94,163,215,41,186,86,49,252,18,40,157,193,89,61,201,209,32,
55,168,136,103,180,48,239,162,80,237,8,187,55,170,15,234,100,87,20,181,176,25,
164,204,234,114,220,141,20,156,125,86,184,54,70,164,32,101,243,202,29,193,216,
219,22,208,151,225,96,68,107,140,77,119,124,211,113,120,89,210,215,94,131,89,
189,1,163,206,214,70,203,98,12,59,127,110,107,216,45,59,187,243,199,183,198,233,
224,38,169,49,130,233,186,16,81,106,51,152,96,54,125,32,195,84,109,24,114,41,
117,105,24,73,139,176,245,16,118,167,248,153,201,46,59,42,36,233,124,57,104,39,
57,110,7,197,53,9,19,203,65,115,147,144,212,77,213,196,134,182,246,136,200,49,
102,58,192,13,222,219,180,87,71,37,66,250,154,91,31,223,101,11,227,128,5,190,
249,230,50,152,230,208,197,69,51,220,65,38,52,209,122,226,177,170,250,144,86,
154,195,74,71,33,100,155,92,179,25,165,237,253,239,98,237,81,214,175,178,244,
203,166,39,241,86,189,28,180,92,134,42,3,22,30,129,128,34,59,74,43,216,146,78,
152,31,100,171,173,202,39,157,24,158,160,145,45,56,18,182,45,51,19,83,71,9,15,
83,158,204,32,82,213,252,184,247,165,126,183,249,117,238,125,237,78,159,116,
229,200,148,88,79,92,90,171,70,26,204,158,0,240,56,116,27,65,55,73,111,96,159,
164,211,108,169,118,248,214,40,30,39,128,115,178,67,82,34,202,81,109,107,31,66,
125,124,53,160,89,27,50,18,3,4,144,55,137,5,201,216,49,80,182,109,43,198,141,76,
93,2,62,146,44,52,137,243,40,202,17,76,186,156,35,139,96,26,194,118,148,89,54,
70,221,165,71,127,144,92,138,102,245,48,200,185,186,129,108,8,116,149,36,179,
182,233,159,246,250,106,43,164,104,35,6,20,178,147,40,7,198,114,45,71,173,83,40,
128,28,105,97,203,209,145,72,108,75,97,146,32,253,194,117,207,157,242,115,128,
127,136,239,130,202,189,43,125,182,18,171,159,4,182,40,125,67,48,109,194,182,77,
61,226,119,205,212,187,236,240,137,135,85,211,18,248,134,151,157,11,219,86,144,
105,142,48,51,169,7,27,254,66,122,204,70,7,231,16,97,162,156,53,224,41,86,144,
218,144,68,108,11,206,21,186,214,196,236,148,73,192,200,192,54,129,74,84,136,
107,19,249,57,179,0,101,31,106,5,73,81,22,100,222,26,34,138,158,97,144,142,14,
236,20,234,167,43,131,248,232,17,176,27,16,148,61,26,36,76,219,2,125,154,204,74,
73,65,209,224,0,61,240,94,178,52,168,177,15,196,4,92,193,84,212,83,219,249,135,
131,62,27,83,127,113,208,103,254,169,254,226,160,207,252,83,253,197,65,111,94,
200,148,152,75,148,163,75,138,8,58,186,215,49,131,240,192,79,232,193,234,44,112,
3,189,120,191,241,126,180,238,132,14,94,246,146,164,102,160,52,44,65,87,73,
241,217,27,127,100,101,34,201,181,13,112,38,14,134,85,146,99,11,21,3,101,4,30,
24,113,77,108,37,25,105,108,11,202,62,215,92,43,74,237,178,214,92,86,170,176,6,
191,155,254,128,72,217,19,211,187,107,142,151,103,233,83,167,85,170,192,88,193,
126,249,29,191,137,85,54,172,110,171,111,154,213,87,162,215,13,238,214,151,242,
98,239,186,201,150,217,253,13,146,24,248,36,225,161,106,53,251,144,37,134,20,
207,99,138,163,233,214,119,246,220,29,108,217,25,187,118,78,129,181,27,118,73,
151,127,102,73,255,178,137,56,136,19,74,54,14,134,94,91,121,159,168,151,221,181,
159,72,103,19,175,30,164,243,154,242,69,184,44,240,130,116,163,62,40,63,233,94,
84,203,70,226,29,221,175,201,190,17,173,94,91,202,239,132,174,30,104,127,67,
232,234,223,212,255,76,232,234,129,250,55,132,174,46,169,191,43,116,245,217,212,
255,42,116,245,148,246,63,8,93,125,54,245,191,10,93,61,165,254,119,66,175,107,
23,66,182,210,234,190,40,160,150,194,114,55,20,94,132,179,64,13,238,70,117,212,
170,81,74,96,67,160,143,36,210,168,90,3,215,235,46,251,225,177,52,57,140,79,146,
240,33,219,228,168,56,9,111,246,208,192,90,128,12,16,157,133,66,39,79,205,64,77,
73,126,211,196,189,179,208,87,175,43,253,31,116,3,192,100,20,153,235,60,149,
138,205,232,219,100,20,210,107,92,83,141,255,88,238,213,12,234,138,68,18,170,
205,75,75,131,234,34,202,97,249,78,125,1,58,11,121,163,126,210,4,108,244,90,69,
20,54,83,171,77,116,97,119,91,83,118,250,71,145,174,80,222,135,47,122,82,148,4,
90,46,135,179,147,181,83,17,203,145,87,122,36,205,91,178,42,5,144,51,150,250,
47,80,22,162,186,33,245,74,18,74,64,113,158,203,102,42,82,55,121,98,53,16,244,
110,174,118,167,119,235,98,89,47,149,80,249,110,12,28,47,205,61,124,13,197,80,
54,124,28,131,129,13,233,221,165,221,187,147,227,19,99,119,72,159,49,116,110,
174,50,162,190,12,9,75,90,234,200,72,2,188,59,10,227,135,148,145,210,122,148,99,
31,211,237,131,43,190,238,38,200,33,45,214,150,119,138,6,141,62,14,62,140,213,
214,70,18,91,255,196,193,33,13,60,99,213,234,224,173,51,43,37,136,163,108,97,75,
227,65,206,206,109,5,104,247,114,68,182,1,228,12,72,87,185,167,112,157,137,89,
167,144,80,21,24,76,186,143,231,121,155,159,41,66,189,212,132,244,10,70,179,113,
222,29,52,121,170,133,26,246,201,186,178,187,141,75,218,167,26,246,208,231,145,
239,181,160,30,212,176,229,188,164,188,142,48,74,123,195,164,23,215,31,165,175,
62,206,245,44,225,63,147,253,135,124,197,230,63,36,255,112,93,221,9,254,94,238,
159,165,126,47,243,45,84,17,169,192,159,83,222,234,201,192,51,89,191,146,244,
205,222,213,115,131,255,189,189,171,231,6,255,123,123,87,207,13,254,247,246,174,
158,27,252,239,237,93,61,218,244,32,152,83,103,26,57,56,225,10,33,83,142,222,
219,25,60,214,18,22,251,153,152,63,227,10,112,212,145,77,78,168,244,80,155,85,
148,43,214,82,161,65,166,244,98,71,223,205,147,78,141,227,165,81,56,146,183,69,
155,204,250,146,170,8,198,81,158,243,202,148,56,221,155,106,82,203,35,79,51,128,
53,229,135,27,95,175,103,144,214,89,232,155,214,242,143,59,203,152,131,210,219,
65,195,185,21,32,74,184,34,127,95,167,248,41,85,99,107,24,99,183,82,155,73,65,
110,87,167,61,195,230,161,7,241,159,232,108,149,47,115,53,161,153,179,10,185,66,
165,114,98,255,165,49,10,200,243,104,142,197,198,165,172,217,103,126,206,83,37,
183,15,169,101,77,159,62,114,92,159,88,243,31,110,177,39,239,169,119,19,37,30,
125,89,251,111,75,171,175,107,255,109,105,245,19,178,127,178,180,250,155,68,254,
191,20,246,39,215,82,31,190,53,93,149,222,72,115,65,90,221,84,204,248,78,5,154,
237,198,171,47,58,201,182,155,215,178,12,86,121,158,95,139,122,159,52,84,183,
131,132,97,239,43,126,115,212,240,203,12,194,75,145,126,163,96,104,37,205,229,
243,28,43,200,49,122,129,147,186,202,131,177,57,200,51,61,190,38,128,163,239,5,
232,200,106,144,12,76,42,105,76,47,231,2,228,41,215,117,226,108,130,179,245,206,
86,210,81,234,187,91,30,252,217,44,95,49,112,238,41,215,132,61,44,157,116,16,
228,199,82,234,154,247,238,82,234,99,222,123,75,169,60,255,155,165,212,37,172,
119,151,82,107,171,43,99,103,146,246,60,161,170,15,121,58,172,141,146,8,162,189,
231,230,109,75,37,145,216,14,82,152,78,73,103,35,199,17,35,49,159,28,150,72,17,
210,93,1,176,11,150,207,150,111,137,126,209,16,146,109,27,164,109,106,165,212,
228,65,176,106,117,234,69,142,225,182,213,248,146,62,212,222,192,55,70,170,157,
149,42,21,54,179,24,41,134,68,35,251,6,210,86,148,180,147,163,228,20,121,168,
130,154,76,246,14,190,99,207,197,127,107,237,193,92,164,81,255,124,49,245,40,
201,127,45,214,173,6,208,201,198,102,41,51,13,241,238,74,77,7,229,170,1,90,170,
6,119,144,2,16,19,153,197,146,73,7,63,247,25,233,232,162,180,99,201,37,56,209,
240,84,165,96,157,82,72,29,135,28,22,238,120,82,52,109,106,50,109,50,222,85,99,
116,144,189,157,36,135,20,38,169,197,8,2,144,214,103,216,123,161,53,6,121,172,
174,250,33,106,178,171,128,141,109,111,151,7,202,47,49,72,128,207,126,24,77,62,
65,53,217,246,190,62,209,102,85,85,51,239,135,59,210,184,240,211,57,229,97,130,
130,83,80,84,144,253,102,189,246,33,254,184,148,108,66,253,39,75,169,53,227,63,
88,74,157,51,222,94,74,221,102,220,45,85,101,187,113,135,78,129,14,33,85,2,39,
69,112,13,169,240,95,30,133,4,81,122,57,124,80,180,52,48,113,40,165,137,156,85,
159,145,243,233,25,170,199,87,221,83,4,138,180,131,251,12,155,234,200,152,165,
45,65,201,227,156,178,31,16,102,13,196,1,0,24,247,32,84,207,142,159,153,35,12,
217,185,40,35,30,150,218,88,74,229,23,15,95,169,107,151,140,204,117,136,95,38,
138,110,193,72,160,189,157,51,228,241,212,81,52,70,76,34,112,90,118,163,90,201,
218,89,102,68,34,74,12,71,41,169,168,35,56,222,137,236,116,31,233,176,27,38,127,
247,64,151,126,250,108,153,186,61,92,38,91,113,248,175,110,161,3,145,123,164,36,
119,179,154,14,39,178,121,59,116,175,213,140,129,38,116,53,142,140,20,240,38,99,
113,171,44,15,8,168,33,61,121,214,234,224,246,233,247,134,248,128,158,141,217,
228,241,173,234,152,145,155,94,91,226,181,103,95,90,196,123,171,60,24,90,181,
115,178,197,22,228,161,211,38,93,148,38,127,84,65,158,153,242,3,119,61,8,37,94,
14,189,84,12,40,200,238,10,238,58,154,156,44,41,152,11,74,234,114,186,4,133,4,
98,101,45,182,76,85,196,122,16,178,117,98,116,187,209,51,9,137,27,17,200,254,
239,221,144,68,78,25,146,1,100,218,238,116,168,254,246,4,221,126,196,227,94,32,
234,65,34,219,244,215,40,22,40,227,123,112,11,43,234,111,158,5,84,255,124,24,
208,111,12,242,130,193,27,231,234,171,84,150,219,202,161,192,45,114,66,121,93,
209,60,24,176,84,207,84,58,182,70,234,36,132,94,199,232,53,19,193,71,80,54,73,
186,42,81,14,227,116,170,33,212,156,124,234,13,253,117,39,241,191,246,230,176,
106,87,80,179,141,24,78,211,58,107,233,210,101,217,161,51,123,143,185,171,103,
242,208,223,218,85,212,207,158,79,84,63,125,64,113,137,234,149,145,248,82,1,163,
178,213,107,3,229,151,180,242,200,59,125,64,178,236,9,83,189,80,224,205,40,13,
49,50,183,70,92,64,204,33,79,3,25,49,35,199,2,107,31,70,238,175,94,107,107,104,
11,60,88,39,100,98,66,246,229,72,179,13,1,162,44,150,0,130,82,95,146,172,240,
78,227,140,34,83,198,152,137,66,181,2,135,7,58,160,162,167,216,67,44,242,128,
164,5,123,218,166,77,17,98,74,209,164,93,45,127,56,162,18,73,178,252,213,132,
166,153,17,113,17,164,158,230,217,206,53,40,119,118,138,217,125,30,81,123,179,
158,77,29,117,29,237,145,191,118,242,250,85,125,55,33,202,179,159,135,60,168,
225,156,151,86,111,61,178,219,39,201,228,240,255,244,71,150,237,86,170,198,108,
20,214,96,0,210,58,123,87,28,12,202,97,156,232,49,23,167,143,152,12,57,131,66,
209,122,137,211,195,156,120,68,158,20,112,83,58,2,193,198,46,214,89,179,2,118,
45,80,37,135,93,173,41,64,129,38,192,156,136,141,196,166,95,15,76,189,138,12,
87,234,74,33,38,17,54,21,121,233,250,220,6,60,15,164,152,235,241,59,107,155,
145,3,130,166,47,180,67,48,214,251,132,201,238,212,222,156,92,253,206,170,47,
176,165,65,157,65,206,107,134,42,157,111,98,163,58,131,125,94,102,13,204,146,51,
51,157,143,73,114,93,251,217,242,103,25,250,191,131,159,122,17,253,54,243,199,
98,255,145,123,225,125,115,126,227,91,78,115,169,126,2,189,147,249,27,235,47,
216,123,25,250,212,141,245,27,227,114,50,99,179,46,37,159,22,222,191,229,92,254,
218,78,143,234,19,243,79,163,223,247,138,223,156,171,247,21,191,89,84,239,43,
126,235,93,189,175,248,61,164,190,42,190,103,82,198,66,30,43,70,184,141,60,100,
179,159,184,178,144,199,254,179,61,235,175,121,144,129,165,252,182,123,199,79,
176,204,70,249,160,153,180,143,13,131,189,253,46,88,158,231,185,40,167,130,134,
20,71,231,3,76,242,52,218,33,199,149,65,8,83,54,94,202,240,97,216,214,152,29,
115,206,205,84,249,35,72,114,196,33,10,100,169,18,94,114,255,130,231,212,207,0,
223,191,95,255,163,133,52,234,39,121,170,255,3,38,49,1,30,207,127,67,20,0,0,1,
131,105,67,67,80,73,67,67,32,112,114,111,102,105,108,101,0,0,120,156,125,145,61,
72,195,64,28,197,95,211,138,34,45,14,118,16,113,136,80,157,44,136,138,56,106,
21,138,80,33,212,10,173,58,152,92,250,5,77,26,146,20,23,71,193,181,224,224,199,
98,213,193,197,89,87,7,87,65,16,252,0,113,115,115,82,116,145,18,255,151,20,90,
196,120,112,220,143,119,247,30,119,239,0,161,81,97,154,21,26,7,52,221,54,211,
201,132,152,205,173,138,221,175,8,33,130,48,134,1,153,89,198,156,36,165,224,59,
190,238,17,224,235,93,156,103,249,159,251,115,68,212,188,197,128,128,72,60,203,
12,211,38,222,32,158,222,180,13,206,251,196,81,86,146,85,226,115,226,49,147,46,
72,252,200,117,197,227,55,206,69,151,5,158,25,53,51,233,121,226,40,177,88,236,
96,165,131,89,201,212,136,167,136,99,170,166,83,190,144,245,88,229,188,197,89,
171,212,88,235,158,252,133,225,188,190,178,204,117,154,67,72,98,17,75,144,32,66,
65,13,101,84,96,35,78,171,78,138,133,52,237,39,124,252,131,174,95,34,151,66,
174,50,24,57,22,80,133,6,217,245,131,255,193,239,110,173,194,228,132,151,20,78,
0,93,47,142,243,49,2,116,239,2,205,186,227,124,31,59,78,243,4,8,62,3,87,122,
219,95,109,0,51,159,164,215,219,90,236,8,232,219,6,46,174,219,154,178,7,92,238,
0,3,79,134,108,202,174,20,164,41,20,10,192,251,25,125,83,14,232,191,5,122,215,
188,222,90,251,56,125,0,50,212,85,234,6,56,56,4,70,139,148,189,238,243,238,158,
206,222,254,61,211,234,239,7,7,204,114,124,170,205,88,60,0,0,0,6,98,75,71,68,0,
255,0,255,0,255,160,189,167,147,0,0,0,9,112,72,89,115,0,0,11,19,0,0,11,19,1,0,
154,156,24,0,0,0,7,116,73,77,69,7,229,4,13,12,49,41,0,66,56,54,0,0,1,143,73,68,
65,84,120,218,237,221,49,110,194,48,20,135,241,4,69,189,136,213,189,91,175,214,
35,244,106,221,186,87,190,8,139,187,64,135,54,8,26,129,253,236,252,190,17,17,
48,254,242,231,249,97,162,76,19,0,0,0,0,96,215,204,23,30,47,166,166,205,252,
175,9,41,57,103,83,85,129,148,210,31,7,51,25,177,164,28,76,73,44,8,9,198,114,
237,9,175,95,79,102,233,142,124,60,31,37,196,87,22,8,33,4,132,16,2,66,244,33,
129,215,228,173,104,217,123,73,136,132,196,252,69,32,74,90,37,68,81,7,33,132,
128,16,66,64,8,33,32,132,16,16,2,66,8,1,33,132,128,16,66,80,149,48,59,134,81,
247,215,37,68,66,98,144,222,62,155,190,127,126,127,145,16,4,78,72,148,51,84,66,
16,71,72,196,171,179,90,143,105,233,113,2,206,75,228,148,210,234,117,246,57,
231,18,85,120,55,53,100,75,143,114,158,248,91,143,235,65,208,161,87,25,163,54,
159,138,58,33,32,68,81,31,123,105,44,33,132,128,16,16,210,101,81,183,147,39,33,
132,128,16,108,173,33,173,247,186,71,227,218,206,168,132,248,202,2,33,132,128,
16,66,64,136,62,196,63,10,37,132,16,68,194,13,93,26,114,203,13,93,126,164,180,
30,236,239,147,226,52,248,48,175,247,168,64,44,255,72,78,77,202,157,199,85,130,
126,78,53,68,81,7,33,67,55,134,21,235,195,163,143,221,250,122,85,235,76,237,162,
214,221,146,122,109,105,58,138,144,110,251,155,154,82,102,50,98,73,81,212,21,
245,41,98,199,124,177,163,223,213,178,55,154,140,8,99,90,246,124,54,106,12,65,8,
33,232,163,15,49,95,0,0,0,0,78,124,3,6,172,90,208,195,35,79,61,0,0,0,0,73,69,
78,68,174,66,96,130};
#ifdef _WIN32
#define vsnprintf vsprintf_s
#endif

void write_log(const char *fmt, ...) {
  char message[2048];
  va_list ap;
  
  va_start(ap, fmt);
  vsnprintf(message, sizeof(message), fmt, ap);
  va_end(ap);
  
  if (!log_buf) {
  	log_buf = new Fl_Text_Buffer();
  }
  
  fprintf(stderr, "%s", message);
  
  log_buf->append(message);
}

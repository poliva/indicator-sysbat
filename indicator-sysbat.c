/*
 *   indicator-sysbat
 *   Copyright 2012 Pau Oliva Fora <pof@eslack.org>
 *   Based on indicator-netspeed (public domain): https://gist.github.com/982939
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <libappindicator/app-indicator.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <sensors/sensors.h>


/* update period in seconds */
#define PERIOD 8

#define CPU_TEMP 	"Physical id 0"		/* cpu temperature sensor */
#define PROXIMITY 	"TC0P"			/* Proxymity sensor */

#define MAXLEN 1024

AppIndicator *indicator;
GtkWidget *indicator_menu;

GtkWidget *sysmon_item;
GtkWidget *quit_item;

typedef struct {
	const sensors_chip_name *chip_name_temp;
	const sensors_chip_name *chip_name_prox;
	const sensors_chip_name *chip_name_fan;
	unsigned int number_temp;
	unsigned int number_prox;
	unsigned int number_fan;
} sensor_data;

sensor_data sd;

int get_cpu()
{
	static int cpu_total_old = 0;
	static int cpu_idle_old = 0;

	glibtop_cpu cpu;
	glibtop_get_cpu (&cpu);

	int diff_total = cpu.total - cpu_total_old;
	int diff_idle = cpu.idle - cpu_idle_old;

	cpu_total_old = cpu.total;
	cpu_idle_old = cpu.idle;

	return 100 * (diff_total-diff_idle) / diff_total;
}

int get_battery()
{
	const char *battery_state="/proc/acpi/battery/BAT0/state";
	const char *battery_info="/proc/acpi/battery/BAT0/info";
	char input[MAXLEN], temp[MAXLEN];
	FILE *fd;
	size_t len;
	int remaining_capacity=1, last_full_capacity=67000;

	fd = fopen (battery_state, "r");
	if (fd == NULL) {
		fprintf (stderr,"Could not open battery_state: %s\n", battery_state);
		return (0);
	}

	while ((fgets (input, sizeof (input), fd)) != NULL) {

		if ((strncmp ("remaining capacity:", input, 19)) == 0) {
			strncpy (temp, input + 19,MAXLEN-1);
			len=strlen(temp);
			temp[len+1]='\0';
			remaining_capacity = atoi(temp);
		}
	}

	fclose(fd);

	fd = fopen (battery_info, "r");
	if (fd == NULL) {
		fprintf (stderr,"Could not open battery_info: %s\n", battery_info);
		return (0);
	}

	while ((fgets (input, sizeof (input), fd)) != NULL) {

		if ((strncmp ("last full capacity:", input, 19)) == 0) {
			strncpy (temp, input + 19,MAXLEN-1);
			len=strlen(temp);
			temp[len+1]='\0';
			last_full_capacity = atoi(temp);
		}
	}

	fclose(fd);

	return 100 * remaining_capacity / last_full_capacity;	
}

int get_fan() {

	double val;
	sensors_get_value(sd.chip_name_fan, sd.number_fan, &val);
	return (int)val;
}

double get_temp() {

	double val;
	sensors_get_value(sd.chip_name_temp, sd.number_temp, &val);
	return val;
}

double get_proximity() {

	double val;
	sensors_get_value(sd.chip_name_prox, sd.number_prox, &val);
	return val;
}

int get_memory()
{
	glibtop_mem mem;
	glibtop_get_mem (&mem);
	/* used memory in percents */
	return 100 - 100 * (mem.free + mem.buffer + mem.cached) / mem.total; 
}

void init_sensor_data() {

	const sensors_chip_name *chip_name;
	const sensors_feature *feature;
	const sensors_subfeature *subfeature;
	char *label;
	int a,b,c;

	a=0;
	while ( (chip_name = sensors_get_detected_chips(NULL, &a)) ) {

		b=0;
		while ( (feature = sensors_get_features(chip_name, &b)) ) {

			c=0;
			while ( (subfeature = sensors_get_all_subfeatures(chip_name, feature, &c)) ) {
				label = sensors_get_label(chip_name, feature);
				if ( strcmp(CPU_TEMP, label)==0 ) {
					printf ("Found sensor: %s (cpu temperature)\n", CPU_TEMP);
					sd.chip_name_temp=chip_name;
					sd.number_temp=subfeature->number;
					break;
				}
				if ( strcmp(PROXIMITY, label)==0 ) {
					printf ("Found sensor: %s (proximity temperature)\n", PROXIMITY);
					sd.chip_name_prox=chip_name;
					sd.number_prox=subfeature->number;
					break;
				}
				if (subfeature->type == SENSORS_SUBFEATURE_FAN_INPUT) {
					printf ("Found FAN sensor\n");
					sd.chip_name_fan=chip_name;
					sd.number_fan=subfeature->number;
					break;
				}
			}
		}
	}
}

gboolean update()
{
	int cpu = get_cpu();
	int memory = get_memory();
	int battery = get_battery();
	int fan = get_fan();
	double temp = get_temp();
	double prox = get_proximity();

	gchar *indicator_label = g_strdup_printf("CPU: %02d%% | Temp: %.1fºC | Prox: %.1fºC | Fan: %d RPM | Mem: %02d%% | Bat: %02d%%", cpu, temp, prox, fan, memory, battery);

	app_indicator_set_label(indicator, indicator_label, "indicator-sysbat");
	g_free(indicator_label);

	return TRUE;
}

static void start_sysmon()
{
	GError *err = NULL;
	gchar *argv[] = {"gnome-system-monitor"};
	if( ! g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err))
	{
		fprintf(stderr, "Error: %s\n", err->message);
	}
}

int main (int argc, char **argv)
{
	gtk_init (&argc, &argv);

	indicator_menu = gtk_menu_new();

	sysmon_item = gtk_menu_item_new_with_label("System Monitor");
	gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), sysmon_item);
	g_signal_connect(sysmon_item, "activate", G_CALLBACK (start_sysmon), NULL);    

	GtkWidget *sep = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), sep);

	quit_item = gtk_menu_item_new_with_label("Quit");
	gtk_menu_shell_append(GTK_MENU_SHELL(indicator_menu), quit_item);
	g_signal_connect(quit_item, "activate", G_CALLBACK (gtk_main_quit), NULL);

	gtk_widget_show_all(indicator_menu);

	indicator = app_indicator_new ("sysbat", "", APP_INDICATOR_CATEGORY_SYSTEM_SERVICES);
	app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_label(indicator, "sysbat", "sysbat");
	app_indicator_set_menu(indicator, GTK_MENU (indicator_menu));

	sensors_init(NULL);
	init_sensor_data();

	update();

	/* update period in milliseconds */
	g_timeout_add(1000*PERIOD, update, NULL);

	gtk_main ();

	return 0;
}

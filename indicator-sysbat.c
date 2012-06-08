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
 */

#include <libappindicator/app-indicator.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <sensors/sensors.h>

#define VERSION "1.0"

/* update period in seconds */
#define PERIOD 8

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

char battery_info[MAXLEN];
char battery_state[MAXLEN];

gboolean fan_enabled=FALSE;
gboolean proximity_enabled=FALSE;
gboolean temperature_enabled=FALSE;
gboolean battery_enabled=FALSE;

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
	char input[MAXLEN], temp[MAXLEN];
	FILE *fd;
	size_t len;
	int remaining_capacity=1, last_full_capacity=67000;

	fd = fopen (battery_state, "r");
	if (fd == NULL) {
		fprintf (stderr,"Could not open battery_state: %s\n", battery_state);
		battery_enabled=FALSE;
		return (-1);
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
		return (-1);
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

void init_battery_data() {

	FILE *fd;
	char input[MAXLEN];
	const char *battery_names[] = {"BAT0", "BAT1", "C23A", "C23B"};
	unsigned int i;

	for (i=0; i< sizeof(battery_names)/sizeof(battery_names[0]) && !battery_enabled; i++) {

		sprintf (battery_info, "/proc/acpi/battery/%s/info", battery_names[i]);
		sprintf (battery_state, "/proc/acpi/battery/%s/state", battery_names[i]);

		fd = fopen (battery_info, "r");
		if (fd == NULL) 
			continue;

		while ((fgets (input, sizeof (input), fd)) != NULL) {

			if ((strncmp ("last full capacity:", input, 19)) == 0) {
				battery_enabled=TRUE;
				printf("Found battery info at /proc/acpi/battery/%s/\n", battery_names[i]);
			}
		}

		fclose(fd);
	}
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
				if ( strcmp("Physical id 0", label)==0 || strcmp("Core 0", label)==0 || strcmp("temp1", label)==0 ) {
					printf ("Found temperature sensor\n");
					sd.chip_name_temp=chip_name;
					sd.number_temp=subfeature->number;
					temperature_enabled=TRUE;
					break;
				}
				if ( strcmp("TC0P", label)==0 ) {
					printf ("Found proximity temperature sensor\n");
					sd.chip_name_prox=chip_name;
					sd.number_prox=subfeature->number;
					proximity_enabled=TRUE;
					break;
				}
				if (subfeature->type == SENSORS_SUBFEATURE_FAN_INPUT) {
					printf ("Found FAN sensor\n");
					sd.chip_name_fan=chip_name;
					sd.number_fan=subfeature->number;
					fan_enabled=TRUE;
					break;
				}
			}
		}
	}
}

gboolean update()
{
	char label[100] = {0};
	int cpu, memory, battery, fan;
	double temp, prox;

	cpu = get_cpu();
	memory = get_memory();

	if (battery_enabled) battery = get_battery();
	if (fan_enabled) fan = get_fan();
	if (temperature_enabled) temp = get_temp();
	if (proximity_enabled) prox = get_proximity();

	sprintf (label, "CPU: %02d%% | ", cpu);
	if (temperature_enabled) sprintf (label, "%sTemp: %.1fºC | ", label, temp);
	if (proximity_enabled) sprintf (label, "%sProx: %.1fºC | ", label, prox);
	if (fan_enabled) sprintf (label, "%sFan: %d RPM | ", label, fan);
	sprintf (label, "%sMem: %02d%%", label, memory);
	if (battery_enabled) sprintf (label, "%s | Bat: %02d%%", label, battery);

	app_indicator_set_label(indicator, label, "indicator-sysbat");

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

	printf("indicator-sysbat v%s - (c)2012 Pau Oliva Fora <pof@eslack.org>\n", VERSION);
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
	init_battery_data();

	update();

	/* update period in milliseconds */
	g_timeout_add(1000*PERIOD, update, NULL);

	gtk_main ();

	return 0;
}

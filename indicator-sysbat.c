/*

This is an Ubuntu appindicator that displays
CPU load, memory usage and % of battery left.

Based on indicator-netspeed:
https://gist.github.com/982939

License: this software is in the public domain.

*/

#include <libappindicator/app-indicator.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>


/* update period in seconds */
#define PERIOD 8

#define MAXLEN 1024

AppIndicator *indicator;
GtkWidget *indicator_menu;

GtkWidget *sysmon_item;
GtkWidget *quit_item;

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

int get_memory()
{
	glibtop_mem mem;
	glibtop_get_mem (&mem);
	/* used memory in percents */
	return 100 - 100 * (mem.free + mem.buffer + mem.cached) / mem.total; 
}

gboolean update()
{
	int cpu = get_cpu();
	int memory = get_memory();
	int battery = get_battery();

	gchar *indicator_label = g_strdup_printf("CPU: %02d%% | Mem: %02d%% | Bat: %02d%%", cpu, memory, battery);

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

	update();

	/* update period in milliseconds */
	g_timeout_add(1000*PERIOD, update, NULL);

	gtk_main ();

	return 0;
}

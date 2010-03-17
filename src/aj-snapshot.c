/************************************************************************/
/* Copyright (C) 2009 Lieven Moors                                      */
/*                                                                      */
/* This file is part of aj-snapshot.                                    */
/*                                                                      */
/* aj-snapshot is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* aj-snapshot is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with Flarps.  If not, see <http://www.gnu.org/licenses/>.      */
/************************************************************************/

#include "aj-snapshot.h"
#include "aj-alsa.h"
#include "aj-file.h"
#include "aj-jack.h"
#include "aj-remove.h"

static void usage(void)
{
	fprintf(stdout, "---------------------------------------------------------------------------------------\n");
	fprintf(stdout, "  aj-snapshot: Store/restore JACK and/or ALSA midi connections to/from an xml file     \n");
	fprintf(stdout, "               Copyright (C) 2010 Lieven Moors                                         \n");
	fprintf(stdout, "---------------------------------------------------------------------------------------\n");
	fprintf(stdout, "   Without options -a or -j, all actions apply to both ALSA and JACK connections       \n");
	fprintf(stdout, "---------------------------------------------------------------------------------------\n");
	fprintf(stdout, "   Usage: aj-snapshot [-options] [file]                                                \n");
	fprintf(stdout, "---------------------------------------------------------------------------------------\n");
	fprintf(stdout, "   -a,--alsa     only store/restore ALSA midi connections                              \n");
	fprintf(stdout, "   -j,--jack     only store/restore JACK audio and midi connections                    \n");
	fprintf(stdout, "   -r,--restore  restore ALSA and/or JACK connections                                  \n");
	fprintf(stdout, "   -x,--remove   with 'file': remove all ALSA and/or JACK connections before restoring \n");
	fprintf(stdout, "   -x,--remove   without 'file': only remove ALSA and/or JACK connections              \n");
	fprintf(stdout, "---------------------------------------------------------------------------------------\n");
} 

enum sys {
	ALSA, JACK, ALSA_JACK
};

enum act {
	STORE, RESTORE, REMOVE_ONLY
};

static const struct option long_option[] = {
	{"help", 0, NULL, 'h'},
	{"alsa", 0, NULL, 'a'},
	{"jack", 0, NULL, 'j'},
	{"restore", 0, NULL, 'r'},
	{"remove-connections", 0, NULL, 'x'},
};

int main(int argc, char **argv)
{ 
	int c;
	int try_remove = 0;
	int remove_connections = 0;
	enum sys system = ALSA_JACK;
	enum act action = STORE;
	static const char *filename;
	mxml_node_t* xml_node = NULL;
	snd_seq_t* seq = NULL;
	jack_client_t* jackc = NULL;

	while ((c = getopt_long(argc, argv, "ajrxh", long_option, NULL)) != -1) {

		switch (c){

		case 'a':
			system = ALSA;
			break;
		case 'j':
			system = JACK;
			break;
		case 'r':
			action = RESTORE;
			break;
		case 'x':
			try_remove = 1;
			break;
		case 'h':
			usage();
			return 1;
		default:
			usage();
			return 1;
		}
	}

	if (argc == (optind + 1)) {
		filename = argv[optind];
	}
	else if ((argc == optind) && try_remove){
		action = REMOVE_ONLY;
	}
	else {
		fprintf(stderr, "-----------------------------------------------------\n");
		fprintf(stderr, "Please specify one file to store/restore the snapshot,\n");
		fprintf(stderr, "or use option -x to remove connections only\n");
		fprintf(stderr, "-----------------------------------------------------\n");
		usage();
		return 1;
	}
	
	if(try_remove){
		if (action != STORE){
			remove_connections = 1;
		}
		else {
			fprintf(stderr, "------------------------------------------------------\n");
			fprintf(stderr, "Will not remove connections before storing connections\n");
			fprintf(stderr, "------------------------------------------------------\n");
		}
	}

	switch (system) {
		case ALSA:
			seq = alsa_initialize(seq);
			if(remove_connections){
				alsa_remove_connections(seq);
				fprintf(stdout, "all ALSA connections removed!\n");
			}
			switch (action){
				case STORE:
					xml_node = mxmlNewXML("1.0");
					alsa_store(seq, xml_node);
					write_xml(filename, xml_node);
					fprintf(stdout, "ALSA connections stored!\n");
					break;
				case RESTORE:
					xml_node = read_xml(filename, xml_node);
					alsa_restore(seq, xml_node);
					fprintf(stdout, "ALSA connections restored!\n");
					break;
				case REMOVE_ONLY:
					break;
			}
			snd_seq_close(seq);
			break;
		case JACK:
			jackc = jack_initialize(jackc);
			if(remove_connections){
				jack_remove_connections(jackc);
				fprintf(stdout, "all JACK connections removed!\n");
			}
			switch (action){
				case STORE:
					xml_node = mxmlNewXML("1.0");
					jack_store(jackc, xml_node);
					write_xml(filename, xml_node);
					mxmlDelete(xml_node);
					fprintf(stdout, "JACK connections stored!\n");
					break;
				case RESTORE:
					xml_node = read_xml(filename, xml_node);
					jack_restore(jackc, xml_node);
					mxmlDelete(xml_node);
					fprintf(stdout, "JACK connections restored!\n");
					break;
				case REMOVE_ONLY:
					break;
			}
			break;
			jack_client_close(jackc);
		case ALSA_JACK:
			seq = alsa_initialize(seq);
			jackc = jack_initialize(jackc);
			if(remove_connections){
				alsa_remove_connections(seq);
				jack_remove_connections(jackc);
				fprintf(stdout, "all ALSA & JACK connections removed!\n");
			}
			switch (action){
				case STORE:
					xml_node = mxmlNewXML("1.0");
					alsa_store(seq, xml_node);
					jack_store(jackc, xml_node);
					write_xml(filename, xml_node);
					mxmlDelete(xml_node);
					fprintf(stdout, "ALSA & JACK connections stored!\n");
					break;
				case RESTORE:
					xml_node = read_xml(filename, xml_node);
					alsa_restore(seq, xml_node);
					jack_restore(jackc, xml_node);
					mxmlDelete(xml_node);
					fprintf(stdout, "ALSA & JACK connections restored!\n");
					break;
				case REMOVE_ONLY:
					break;
			}
			snd_seq_close(seq);
			jack_client_close(jackc);
			break;
	}
	return 0;
}
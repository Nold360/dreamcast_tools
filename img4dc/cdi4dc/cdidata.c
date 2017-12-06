/* 
	:: C D I 4 D C ::
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	13 april 2007, v0.3b
	
	File : 	CDIDATA.C
	Desc : 	Contains all Data/Data image related.
			
			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
*/

#include "cdibuild.h"

void write_data_gap_start_track(FILE* cdi) {
	int i;
	unsigned char* buf;
	
	buf = (char *) malloc(gap_sector_size);
	
	// �crire Gap 1
	fill_buffer(buf, gap_sector_size, gap_dummy_sector_size, gap_dummy_sector);
	
	for(i = 0 ; i < gap_data_start_image_count ; i++) {
		fwrite(buf, 1, gap_sector_size, cdi);
	}
		
	free(buf);	
}

void write_data_header_boot_track(FILE* cdi, FILE* iso) {
	unsigned char buf[(LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME + (L2_RAW + L2_Q + L2_P) * FRAMES_PER_SECTOR];
	int block_count = 0;
	unsigned int address = encode_address;
	
	// d�but piste
	write_null_block(cdi, 8);
	
	// �crire l'IP.BIN (secteur 0), le 17�me et 18�me secteur de l'ISO (18 secteurs � �crire en tout, num�rot�s de 0 � 17)
	fseek(iso, 0x0, SEEK_SET); // 17�me secteur commence � 0x8000. IP.BIN commence � 0x0.
	
	block_count = 0; // un bloc de plus que la boucle + encore 1
	while((block_count < 18) && (fread(buf + load_offset, sect_size[0][1], 1, iso))) {
		block_count++;
		
		do_encode_L2(buf, MODE_2_FORM_1, address);
		
		// on �crit le secteur encod�. On d�cale de 24 offset (load_offset) afin de ne pas �crire le d�but du secteur (inutile ici).
		fwrite(buf + 24, sect_size[0][0] - 16, 1, cdi);
		
		address++;
	}
		
	// padding
	while(block_count < 300) {
		write_null_block(cdi, sect_size[0][0] - 16); // on complete donc par des z�ros binaires
		block_count++;
	}
	
	// ecrire deux secteurs GAP 1
	write_gap_end_tracks(cdi);
}


// ecrire le secteur 1 (donn�es) du header CDI. il contient le nombre de secteurs de donn�es
void write_cdi_header_data_data_sector(FILE* cdi, long data_sector_count) {
	unsigned char *buf;
		
	buf = (char *) malloc(cdi_head_data_data_track_sector_size);
	fill_buffer(buf, cdi_head_data_data_track_sector_size, cdi_head_data_data_track_sector_entries, cdi_head_data_data_track_sector);
	
	buf[0x06] = data_sector_count;
	buf[0x07] = data_sector_count >> 8;
	buf[0x08] = data_sector_count >> 16;
	buf[0x09] = data_sector_count >> 31;
	
	buf[0x24] = data_sector_count + 150;
	buf[0x25] = (data_sector_count + 150) >> 8;
	buf[0x26] = (data_sector_count + 150) >> 16;
	buf[0x27] = (data_sector_count + 150) >> 31;
	
	buf[0x41] = data_sector_count + 150;
	buf[0x42] = (data_sector_count + 150) >> 8;
	buf[0x43] = (data_sector_count + 150) >> 16;
	buf[0x44] = (data_sector_count + 150) >> 31;
	
	fwrite(buf, sector2_size, 1, cdi);
	free(buf);
}

#define NEXT_TRACK_LBA 11400
void write_cdi_header_data_data_sector_2(FILE* cdi, long data_sector_count) {
	unsigned char *buf;
	int header_track_lba;
	
	buf = (char *) malloc(cdi_head_data_data_track_2_sector_size);
	fill_buffer(buf, cdi_head_data_data_track_2_sector_size, cdi_head_data_data_track_2_sector_entries, cdi_head_data_data_track_2_sector);
	
	header_track_lba = data_sector_count + NEXT_TRACK_LBA;
	
	buf[0x20] = header_track_lba;
	buf[0x21] = header_track_lba >> 8;
	buf[0x22] = header_track_lba >> 16;
	buf[0x23] = header_track_lba >> 31;
	
	buf[0xb8] = header_track_lba;
	buf[0xb9] = header_track_lba >> 8;
	buf[0xba] = header_track_lba >> 16;
	buf[0xbb] = header_track_lba >> 31;
	
	fwrite(buf, sector2_size, 1, cdi);
	free(buf);
}

// ecrire l'header � la fin du fichier. Cette fonction appelle toutes les autres situ�es ci dessus !
int write_data_cdi_header(FILE *cdi, char* cdiname, char* volume_name, long data_sector_count, long total_cdi_space_used) {
	
	struct cdi_header head;
	unsigned long cdi_end_image_tracks;
	
	cdi_end_image_tracks = ftell(cdi); // emplacement de l'header
	
	// en t�te
	head.track_count = 0x02;
	head.first_track_num = 0x01;
	head.padding = 0x00000000;
	
#ifdef WIN32
	get_full_filename(cdiname, cdiname); // on remplace l'ancien nom relatif par le nom absolu
#endif
	
	// ecrire le d�but de l'header
	fwrite(&head, sizeof(head), 1, cdi);
	
	// sector 1 (user data)
	write_cdi_header_start(cdi, cdiname);
	write_cdi_header_data_data_sector(cdi, data_sector_count);
	
	// sector 2 (header data)
	write_cdi_header_start(cdi, cdiname);
	write_cdi_header_data_data_sector_2(cdi, data_sector_count);
	
	// sector 3 (fin de l'header)
	write_cdi_header_start(cdi, cdiname);
	write_cdi_head_end(cdi, volume_name, total_cdi_space_used, cdi_end_image_tracks);
}

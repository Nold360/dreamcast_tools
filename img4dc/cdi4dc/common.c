/* 
	:: C D I 4 D C ::
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	13 april 2007, v0.3b
	
	File : 	COMMON.C
	Desc : 	Common part of the CDI generation.
			
			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
*/

#include "cdibuild.h"

// ecrire la piste pregap entre l'audio et le data
int write_gap_tracks(FILE* cdi) {
	int i;
	unsigned char* buf;
	
	buf = (char *) malloc(gap_sector_size);
	
	// �crire Gap 1
	fill_buffer(buf, gap_sector_size, gap_dummy_sector_size, gap_dummy_sector);
	
	for(i = 0 ; i < gap_sector_count ; i++) {
		fwrite(buf, 1, gap_sector_size, cdi);
	}
	
	// �crire Gap 2
	fill_buffer(buf, gap_sector_size, gap_dummy_sector_2_size, gap_dummy_sector_2);
	
	for(i = 0 ; i < gap_sector_count ; i++) {
		fwrite(buf, 1, gap_sector_size, cdi);
	}
	
	free(buf);
	
	return 0;
}

// ecrire des pistes gap entre les donn�es et l'header du cdi situ� � la fin du fichier
int write_gap_end_tracks(FILE *cdi) {
	unsigned char *buf;
	int i;
	
	buf = (char *) malloc(gap_sector_size);
	fill_buffer(buf, gap_sector_size, gap_dummy_sector_size, gap_dummy_sector);
	
	fseek(cdi, ftell(cdi) - 8, SEEK_SET);
	
	// 2 secteurs de type GAP 1
	for(i = 0 ; i < 2 ; i++)
		fwrite(buf, 1, gap_sector_size, cdi);
	
	free(buf);
}

// ecrire la piste de donn�es
int write_data_track(FILE* cdi, FILE* iso) {

	int length, block_count = 0;
	unsigned int address = encode_address;
	unsigned long iso_size;
	
	iso_size = fsize(iso);
	
	unsigned char buf[(LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME + (L2_RAW + L2_Q + L2_P) * FRAMES_PER_SECTOR];
	
	// d�but piste data
	write_null_block(cdi, 8);
		
	block_count = 2; // un bloc de plus que la boucle + encore 1
	while(length = fread(buf + load_offset, sect_size[0][1], 1, iso)) {
		block_count++;
		
		do_encode_L2(buf, MODE_2_FORM_1, address);
		
		// on �crit le secteur encod�. On d�cale de 24 offset (load_offset) afin de ne pas �crire le d�but du secteur (inutile ici).
		fwrite(buf + 24, sect_size[0][0] - 16, 1, cdi);
		
		address++;
		
		// c'est pour l'affichage graphique
		writing_data_track_event((block_count - 2) * L2_RAW, iso_size);
	}
	
	// piste data trop petite ! Il faut qu'elle fasse au minimum 302 secteurs.
	if (block_count < 302) padding_event(block_count); // se produit qu'une fois. C'est juste pour l'afficher � l'utilisateur.
	
	// ici on commence vraiment l'op�ration
	while(block_count < 302) {
		write_null_block(cdi, sect_size[0][0] - 16); // on complete donc par des z�ros binaires
		block_count++;
	}
	
	// ecrire deux secteurs GAP 1
	write_gap_end_tracks(cdi);
	
	return block_count;
}

// ecrire le d�but de l'header (contenant le nom du fichier)
void write_cdi_header_start(FILE* cdi, char* cdiname) {
	
	int i;
	unsigned char cdi_filename_length; // longeur de la chaine qui va suivre, elle contient le nom complet du fichier CDI.
	unsigned char head_track_start_mark_blocks[20]; // deux fois la m�me valeur (track_start_mark)
	unsigned short int unknow, unknow2;
	
	// remplir le tableau contenant les octets repr�sentant le d�but d'une piste
	for(i = 0 ; i < 20 ; i++) {
		head_track_start_mark_blocks[i] = track_start_mark[i % 10];
	}
	
	// ecrire deux fois l'en-t�te de la piste
	fwrite(head_track_start_mark_blocks, sizeof(head_track_start_mark_blocks), 1, cdi);
		
	// valeur inconnue ... pour le moment. Il semblerait qu'elle soit importante et qu'elle d�pend du nom du fichier enregistr� dans l'header...
	unknow = 0x00AB; // pfff impossible de savoir ce que c'est !!!
	fwrite(&unknow, sizeof(unknow), 1, cdi);
	
	unknow2 = 0x0210; //constante... � quoi elle sert ? on s'en fout elle y est on la remet...
	fwrite(&unknow2, sizeof(unknow2), 1, cdi);
	
	cdi_filename_length = strlen(cdiname);
	fwrite(&cdi_filename_length, sizeof(cdi_filename_length), 1, cdi); // ecrire la taille
	fwrite(cdiname, cdi_filename_length, 1, cdi); // ecrire le nom du fichier destination CDI
	
	// ecrire la suite apr�s le nom du fichier... (octets intermin�s)
	write_array_block(cdi, cdi_head_next_size, cdi_head_next_entries, cdi_head_next);
}

// ecrire la fin de l'header CDI (contient le nom du volume, la taille totale des secteurs du CDI et une valeur servant � calculer l'emplacement de l'header du CDI).
void write_cdi_head_end(FILE* cdi, char* volumename, long total_cdi_space_used, long cdi_end_image_tracks) {
	
	int i;
	unsigned char volumename_length;
	unsigned long cdi_header_pos;
	
	volumename_length = strlen(volumename);
	
	// ecrire la taille utilis�e au total sur le disque
	fwrite(&total_cdi_space_used, sizeof(total_cdi_space_used), 1, cdi);
	
	// ecrire la taille du volume
	fwrite(&volumename_length, sizeof(volumename_length), 1, cdi);
	
	// ecrire le nom du volume
	fwrite(volumename, volumename_length, 1, cdi);
	
	// ecrire les octets de fin
	write_array_block(cdi, cdi_head_end_size, cdi_head_end_entries, cdi_header_end);
	
	// ecrire la position de l'header (les 4 derniers octets du fichier)
	cdi_header_pos = (ftell(cdi) + 4) - cdi_end_image_tracks; // ftell + 4 = taille du fichier image
	fwrite(&cdi_header_pos, sizeof(cdi_header_pos), 1, cdi);
}

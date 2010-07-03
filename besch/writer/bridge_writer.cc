#include <string>
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../bruecke_besch.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "bridge_writer.h"

using std::string;

void bridge_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 22, &parent);

	uint8  wegtyp        = get_waytype(obj.get("waytype"));
	uint16 topspeed      = obj.get_int("topspeed", 999);
	uint32 preis         = obj.get_int("cost", 0);
	uint32 maintenance   = obj.get_int("maintenance", 1000);
	uint8  pillars_every = obj.get_int("pillar_distance",0); // distance==0 is off
	uint8  pillar_asymmetric = obj.get_int("pillar_asymmetric",0); // middle of tile
	uint8  max_length    = obj.get_int("max_lenght",0); // max_lenght==0: unlimited
	max_length    = obj.get_int("max_length",max_length); // with correct spelling
	uint8  max_height    = obj.get_int("max_height",0); // max_height==0: unlimited

	// prissi: timeline
	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 obsolete_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	obsolete_date += obj.get_int("retire_month", 1) - 1;

	sint8 number_seasons = 0;

	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version = 0x8008;
	node.write_uint16(outfp, version,            0);
	node.write_uint16(outfp, topspeed,           2);
	node.write_uint32(outfp, preis,              4);
	node.write_uint32(outfp, maintenance,        8);
	node.write_uint8 (outfp, wegtyp,            12);
	node.write_uint8 (outfp, pillars_every,     13);
	node.write_uint8 (outfp, max_length,        14);
	node.write_uint16(outfp, intro_date,        15);
	node.write_uint16(outfp, obsolete_date,     17);
	node.write_uint8 (outfp, pillar_asymmetric, 19);
	node.write_uint8 (outfp, max_height,        20);

	static const char* const names[] = {
		"image",
		"ns", "ew", NULL,
		"start",
		"n", "s", "e", "w", NULL,
		"ramp",
		"n", "s", "e", "w", NULL,
		NULL
	};
	slist_tpl<string> backkeys;
	slist_tpl<string> frontkeys;

	slist_tpl<string> cursorkeys;
	cursorkeys.append(string(obj.get("cursor")));
	cursorkeys.append(string(obj.get("icon")));

	char keybuf[40];

	string str = obj.get("backimage[ns][0]");
	if(str.size() == 0) {
		node.write_data_at(outfp, &number_seasons, 21, sizeof(uint8));
		write_head(outfp, node, obj);
		const char* const * ptr = names;
		const char* keyname = *ptr++;

		do {
			const char* keyindex = *ptr++;
			do {
				string value;

				sprintf(keybuf, "back%s[%s]", keyname, keyindex);
				value = obj.get(keybuf);
				backkeys.append(value);
				//intf("BACK: %s -> %s\n", keybuf, value.chars());
				sprintf(keybuf, "front%s[%s]", keyname, keyindex);
				value = obj.get(keybuf);
				if (value.size() > 2) {
					frontkeys.append(value);
					//intf("FRNT: %s -> %s\n", keybuf, value.chars());
				} else {
					printf("WARNING: not %s specified (but might be still working)\n",keybuf);
				}
				keyindex = *ptr++;
			} while (keyindex);
			keyname = *ptr++;
		} while (keyname);

		if (pillars_every > 0) {
			backkeys.append(string(obj.get("backpillar[s]")));
			backkeys.append(string(obj.get("backpillar[w]")));
		}
		imagelist_writer_t::instance()->write_obj(outfp, node, backkeys);
		imagelist_writer_t::instance()->write_obj(outfp, node, frontkeys);
		cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
		backkeys.clear();
		frontkeys.clear();
	} else {
		while(number_seasons < 2) {
			sprintf(keybuf, "backimage[ns][%d]", number_seasons+1);
			string str = obj.get(keybuf);
			if(str.size() > 0) {
				number_seasons++;
			} else {
				break;
			}
		}

		node.write_data_at(outfp, &number_seasons, 21, sizeof(uint8));
		write_head(outfp, node, obj);

		for(uint8 season = 0 ; season <= number_seasons ; season++) {
			const char* const * ptr = names;
			const char* keyname = *ptr++;

			do {
				const char* keyindex = *ptr++;
				do {
					string value;

					sprintf(keybuf, "back%s[%s][%d]", keyname, keyindex, season);
					value = obj.get(keybuf);
					backkeys.append(value);
					//intf("BACK: %s -> %s\n", keybuf, value.chars());
					sprintf(keybuf, "front%s[%s][%d]", keyname, keyindex, season);
					value = obj.get(keybuf);
					if (value.size() > 2) {
						frontkeys.append(value);
						//intf("FRNT: %s -> %s\n", keybuf, value.chars());
					} else {
						printf("WARNING: not %s specified (but might be still working)\n",keybuf);
					}
					keyindex = *ptr++;
				} while (keyindex);
				keyname = *ptr++;
			} while (keyname);

			if (pillars_every > 0) {
				sprintf(keybuf, "backpillar[s][%d]",season);
				backkeys.append(string(obj.get(keybuf)));
				sprintf(keybuf, "backpillar[w][%d]",season);
				backkeys.append(string(obj.get(keybuf)));
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, backkeys);
			imagelist_writer_t::instance()->write_obj(outfp, node, frontkeys);
			if(season == 0 ) {
				cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
			}
			backkeys.clear();
			frontkeys.clear();
		}
	}

	cursorkeys.clear();

	// node.write_data(outfp, &besch);
	node.write(outfp);
}

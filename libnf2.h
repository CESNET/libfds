
// Prace se souborem -----------------------------------------------------------

// Otevreni pro cteni/apendovani/zapis
lnf_ctx_t *
lnf_ctx_new(FILE *file, int flags); // rezim cteni/zapisu

// Uzavreni souboru
void
lnf_ctx_destroy(lnf_ctx_t *ctx);

void
lnf_ctx_file_new(lnf_ctx_t *ctx, FILE *file);

FILE *
lnf_ctx_file_get(lnf_ctx_t *ctx);




// Zapis zaznamu do souboru
// TODO
lnf_ctx_write(lnf_ctx_t *ctx, lnf_exporter_t *exp, lnf_rec_t *rec);

// Cteni zaznamu ze souboru
// TODO
lnf_ctx_read(...);

// TODO: jak cist treba jen vyznamne bloky...


// Prace s exportery -----------------------------------------------------------
// TODO



lnf_exporter_t *
lnf_exporter_add(lnf_ctx_t *ctx, uint32_t odid, uint8_t addr[16],
	const char *description);

// Prace s sablonami -----------------------------------------------------------



// Prace s FLOW zaznamy a sablonami --------------------------------------------
// HIGH level

typedef struct lnf_rec_s lnf_rec_t;

lnf_rec_t *
lnf_rec_init();

void
lnf_rec_destroy(lnf_rec_t *rec);

// Po použití clearu si stale bude pamatovat sablonu podle ktere naposledy
// ulozil data a bude se pri _set snazit vkládat položky dle této šablony. 
// Pri prvni neshode provedem šablon magic
lnf_rec_clear(lnf_rec_t *rec);

// Pozn: dynamická položka se pozna podle vnitrni definice
int
lnf_rec_set(lnf_rec_t *rec, uint32_t f_en, uint16_t f_id, const uint8_t *data,
	uint16_t len);


lnf_rec_get(lnf_rec_t *rec, uint32_t f_en, uint16_t f_id, uint8_t *data, );

// TODO: iterator by se pro cteni mohl hodit 


// Middle level
lnf_template_t *
lnf_template_add(lnf_ctx_t *ctx, uint16_t fld_cnt, const struct lnf_ctx_tmplt_field *fields);

// V pripade, že sablona je NULL bude skladat zaznam dynamicky. Pokud jiz 
// drive tvorit zaznam dynamicky, nebude předpokládat použití stejné šablony.
void
lnf_rec_set_template(lnf_rec_t *rec, lnf_template_t *tmplt);


// Low level

// Vrati ukazatel do bufferu
uint8_t *
lnf_raw_alloc(lnf_ctx_t *ctx, lnf_exporter_t *exp, lnf_template_t *tmplt, uint16_t size);

// Oznami ze zaznam byl v datech vyplnen (pozita delka jsou prvni 2B pouzite pameti)
lnf_raw_finalize(lnf_ctx_t *ctx);


















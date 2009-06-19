#include <SDL/SDL.h>
#include <stdio.h>

#include "graphics.h"
#include "particle.h"
#include "psys.h"
#include "grid.h"
#include "element.h"
#include "config.h"
#include "gui.h"

/* Point-to-grid and grid-to-point version macros */
#define PCRD(x,y) ((x)<<3), ((y)<<3)
#define GCRD(x,y) ((x)>>3), ((y)>>3)

/* GUI macros */
#define LINE(x1,y1,x2,y2) {glVertex2f((x1),(y1)); glVertex2f((x2),(y2));}
#define PLINE(x1,y1,x2,y2) {glVertex2i(PCRD((x1),(y1))); glVertex2i(PCRD((x2),(y2)));}
#define GLINE(x1,y1,x2,y2) {glVertex2i(GCRD((x1),(y1))); glVertex2i(GCRD((x2),(y2)));}

int element_count = 0;
int block_count = 0;

int material_pointer = 0;
int xy_pointer[2] = {1, 1};
int next_key = 0;

void change_selected(int delta)
{
	int total_count = element_count + block_count;
	
    if(material_pointer + delta > total_count - 1) material_pointer = 0;
    else if(material_pointer + delta < 0) material_pointer = total_count - 1;
    else material_pointer += delta;
	
	char* material_name;
	if (material_pointer < element_count) material_name = element_get_name(material_pointer);
	else material_name = block_get_name(material_pointer - element_count);
	
    SDL_WM_SetCaption(material_name, "Fluvium");
}

void set_selected(int index)
{
    material_pointer = index;
    
	char* material_name;
	if (material_pointer < element_count) material_name = element_get_name(material_pointer);
	else material_name = block_get_name(material_pointer - element_count);
	
    SDL_WM_SetCaption(material_name, "Fluvium");
}

void render_arrow()
{
    glBegin(GL_LINES);
    LINE(0.0, -0.4,  0.0, 0.4);
    LINE(0.0, -0.4, -0.4, 0.0);
    LINE(0.0, -0.4,  0.4, 0.0);
    glEnd();
}

void do_key(graphics *gfx, const unsigned int clock)
{
    if((*gfx).key[SDLK_LSHIFT] || (*gfx).key[SDLK_RSHIFT])
    {
        next_key = clock + 40;
    }
    else
    {
        next_key = clock + 140;
    }
}

void put_material()
{
	if(material_pointer < element_count)
	{
		psys_add_element(xy_pointer[0], xy_pointer[1], material_pointer);
	}
	else
	{
		int block_pointer = material_pointer - element_count;
		
		grid_type(xy_pointer[0], xy_pointer[1], block_pointer);
		if(block_pointer == 0)
		{
			psys_delete(xy_pointer[0], xy_pointer[1]);
		}
	}
}

int main(int argc, char **argv)
{
    graphics gfx;
    graphics_init(&gfx, 640, 512, "data/lacurg.ttf");

    gui ui;
    gui_init(&ui, &gfx, 128, 512);

    element_init();
	block_init();
    config_parse(&ui, "./data/config.txt", &element_count, &block_count);
	
	gui_build(&ui);
	
    change_selected(0);

    GLuint grid_list = glGenLists(1);
    glNewList(grid_list, GL_COMPILE);
    glColor3ub(20, 20, 20);
    glBegin(GL_LINES);
    
    int y, x;
    for(y=0; y<64; ++y) PLINE(0, y, 64, y);
    for(x=0; x<64; ++x) PLINE(x, 0, x, 64);
    
    glEnd();
    glEndList();

    psys_init(512, 512, 10000);
	
    while(graphics_events(&gfx))
    {
		long start, frame;
		start = SDL_GetTicks();
		
        glClear(GL_COLOR_BUFFER_BIT);

        for(y=0; y<64; ++y)
        {
            for(x=0; x<64; ++x)
            {
                unsigned char gtype = grid_get_type(x, y);
                if(gtype)
                {
					unsigned short gappearance = block_get_appearance(gtype);
					
                    glPushMatrix();
                    glScalef(8, 8, 1);
					
                    if(gappearance >= A_LEFTARROW && gappearance <= A_DOWNARROW)
                    {
                        glTranslatef(x+0.5, y+0.5, 0.0);
                        if(gappearance == A_DOWNARROW) glRotatef(180, 0, 0, 1);
                        else if(gappearance == A_RIGHTARROW) glRotatef(90, 0, 0, 1);
                        else if(gappearance == A_LEFTARROW) glRotatef(270, 0, 0, 1);
                        glColor3ub(0x00, 0xaa, 0x00);
                        render_arrow();
                    }
                    else if(gappearance == A_COLOR)
                    {
						GLubyte* rgb;
						block_get_rgb(gtype, &rgb);
						
                        glColor3ub(*(rgb), *(rgb+1), *(rgb+2));
						if(gtype == G_EMIT)
                        {
                            glColor3ub(255, 64, 255);
                            particle p;
                            p.vel[0] = 0.0;
                            p.vel[1] = 0.0;
                            p.pos[0] = (x << 3) + (rand() % 8);
                            p.pos[1] = (y << 3) + (rand() % 8);
                            particle_factory(&p, grid_get_data(x, y));
                            psys_add(&p);
                        }
                        glTranslatef(x, y, 0);
                        glBegin(GL_QUADS);
                        glVertex2i(0, 0);
                        glVertex2i(1, 0);
                        glVertex2i(1, 1);
                        glVertex2i(0, 1);
                        glEnd();
                    }
                    glPopMatrix();
                }
            }
        }

        glCallList(grid_list);

        psys_calc();

        gui_render(&ui, material_pointer);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glPushMatrix();
        glScalef(8, 8, 1);
        glTranslatef(xy_pointer[0], xy_pointer[1], 0.0);
        glColor3ub(0xff, 0xff, 0xff);
        glBegin(GL_QUADS);
        glVertex2f(0.0, 0.0);
        glVertex2f(1.0, 0.0);
        glVertex2f(1.0, 1.0);
        glVertex2f(0.0, 1.0);
        glEnd();
        glPopMatrix();
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glPushMatrix();
        glScalef(8, 8, 1);
        glTranslatef(64.7, material_pointer*2+1, 0.0);
        glRotatef(90, 0, 0, 1);
        glColor3ub(0xaa, 0x90, 0x90);
        //render_arrow();
        glRectf(1.0, -15.2, -1.0, 0.6);
		glPopMatrix();		
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		
        SDL_GL_SwapBuffers();

        const unsigned int clock = SDL_GetTicks();

        // Stall when needed to smooth frames
        while(graphics_sync_time(&gfx, 4)){}

        if(next_key < clock)
        {
            if(gfx.key[SDLK_UP]) {xy_pointer[1]--; do_key(&gfx, clock);}
            else if(gfx.key[SDLK_DOWN]) {xy_pointer[1]++; do_key(&gfx, clock);}
            if(gfx.key[SDLK_RIGHT]) {xy_pointer[0]++; do_key(&gfx, clock);}
            else if(gfx.key[SDLK_LEFT]) {xy_pointer[0]--; do_key(&gfx, clock);}

            if(xy_pointer[0] < 0) xy_pointer[0] = 63;
            else if(xy_pointer[0] > 63) xy_pointer[0] = 0;
            if(xy_pointer[1] < 0) xy_pointer[1] = 63;
            else if(xy_pointer[1] > 63) xy_pointer[1] = 0;
        }

        if(gfx.key[SDLK_z])
        {
            grid_type(xy_pointer[0], xy_pointer[1], G_EMIT);
            grid_data(xy_pointer[0], xy_pointer[1], material_pointer);
        }
        else if(gfx.key[SDLK_RETURN])
        {
            put_material();
        }
		else
		{
			int i;
			for(i=0;i<element_count;i++)
			{
				if(gfx.key[element_get_hotkey(i)])
				{
					psys_add_element(xy_pointer[0], xy_pointer[1], i);
					break;
				}
			}
			if (i >= element_count)
			{
				for(i=0;i<block_count;i++)
				{
					if(gfx.key[block_get_hotkey(i)])
					{
						if (i==0) psys_delete(xy_pointer[0], xy_pointer[1]);
						grid_type(xy_pointer[0], xy_pointer[1], i);
						break;
					}
				}
			}
		}

        if(graphics_onkey(&gfx, SDLK_F1))
        {
            config_parse(&ui, "./data/config.txt", &element_count, &block_count);
            gui_build(&ui);
        }


        if(graphics_onkey(&gfx, SDLK_PAGEDOWN))
        {
            change_selected(1);
        }
        else if(graphics_onkey(&gfx, SDLK_PAGEUP))
        {
            change_selected(-1);
        }
        
        if(gfx.but[SDL_BUTTON_LEFT]) //graphics_onbut(&gfx, SDL_BUTTON_LEFT))
        {
            // GUI click or particle click?
            if(gfx.mouse_x > 512)
            {
                if(gfx.mouse_y < (element_count + block_count) * 16)
                {
                    set_selected(gfx.mouse_y / 16);
                }
            }
            else
            {
                xy_pointer[0] = gfx.mouse_x / G_S;
                xy_pointer[1] = gfx.mouse_y / G_S;
				
				put_material();
			}
        }
		frame = SDL_GetTicks() - start;
		//printf("FPS: %f\n", (frame > 0) ? 1000.0f / frame : 0.0f);
    }
    graphics_free(&gfx);
    return 0;
}
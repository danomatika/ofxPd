#include <m_pd.h>
#include <m_fixed.h>

typedef struct mtof_tilde
{
    t_object x_obj;
    float x_f;
} t_mtof_tilde;

t_class *mtof_tilde_class;

static void *mtof_tilde_new(void)
{
    t_mtof_tilde *x = (t_mtof_tilde *)pd_new(mtof_tilde_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *mtof_tilde_perform(t_int *w)
{
    t_sample *in = *(t_sample **)(w+1), *out = *(t_sample **)(w+2);
    t_int n = *(t_int *)(w+3);
    for (; n--; in++, out++)
    {
	t_sample f = *in;
	if (f <= ftofix(-1500.)) *out = 0;
	else
	{
	    if (f > ftofix(1499.)) f = ftofix(1499.);
	    *out = ftofix(8.17579891564 * exp(.0577622650 * fixtof(f)));
    	}
    }
    return (w + 4);
}

static void mtof_tilde_dsp(t_mtof_tilde *x, t_signal **sp)
{
    post("warning: %s not usable yet",__FUNCTION__);
    dsp_add(mtof_tilde_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void mtof_tilde_setup(void)
{
    mtof_tilde_class = class_new(gensym("mtof~"), (t_newmethod)mtof_tilde_new, 0,
    	sizeof(t_mtof_tilde), 0, 0);
    CLASS_MAINSIGNALIN(mtof_tilde_class, t_mtof_tilde, x_f);
    class_addmethod(mtof_tilde_class, (t_method)mtof_tilde_dsp, gensym("dsp"), 0);
}

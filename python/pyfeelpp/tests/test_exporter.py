import sys
import feelpp 
import pytest

def run(m, geo):
    mesh_name, dim, e_meas, e_s_1, e_s_2, e_s_bdy = geo()
    m2d = feelpp.load(m, mesh_name, 0.1)

    Xh = feelpp.functionSpace(space="Pch", mesh=m2d, order=1)
    P0h = feelpp.functionSpace(space="Pdh", mesh=m2d, order=0)
    #u=Xh.elementFromExpr("{sin(2*pi*x)*cos(pi*y)}:x:y")
    u = Xh.element()
    u.on(range=feelpp.elements(m2d), expr=feelpp.expr("x*x:x"))

    e = feelpp.exporter(mesh=m2d, name="feelpp"+str(m.dimension())+"d")
    e.addScalar("un", 1.)
    e.addP1c("u", u)
    e.addP0d("pid", feelpp.pid(P0h))
    e.save()


def test_exporter(init_feelpp):
    feelpp.Environment.changeRepository(
        directory="pyfeelpp-tests/exporter")
    geo={
        '2':feelpp.create_rectangle,
        '3':feelpp.create_box
    }
    run( feelpp.mesh( dim=2 ), geo['2'] )
    run( feelpp.mesh( dim=3, realdim=3 ), geo['3'] )




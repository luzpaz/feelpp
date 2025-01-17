/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*-*/

#define BOOST_TEST_MODULE test_wire_basket
#include <feel/feelcore/testsuite.hpp>

#include <feel/options.hpp>
#include <feel/feelalg/backend.hpp>
#include <feel/feeldiscr/functionspace.hpp>
#include <feel/feelfilters/creategmshmesh.hpp>
#include <feel/feelfilters/savegmshmesh.hpp>
#include <feel/feelfilters/domain.hpp>
#include <feel/feelvf/vf.hpp>
#include <feel/feelfilters/exporter.hpp>
#include <feel/feeldiscr/createsubmesh.hpp>
#include <feel/feeldiscr/projector.hpp>


namespace test_wire_basket
{

using namespace Feel;
/*_________________________________________________*
 * Run
 *_________________________________________________*/

template <uint16_type OrderPoly>
void run()
{

    /* change parameters below */
    const int nDim = 3;
    const int nOrderPoly = OrderPoly;
    double meshSize = doption(_name="gmsh.hsize");

    //--------------------------------------------------------------------------------------------------//

    typedef Mesh< Simplex<nDim,1,nDim> > mesh_type;
    typedef FunctionSpace<mesh_type, bases<Lagrange<OrderPoly,Scalar> > > space_type;
    typedef Exporter<mesh_type> export_type;
    // trace
    typedef typename mesh_type::trace_mesh_type trace_mesh_type;
    typedef Exporter<trace_mesh_type> trace_export_type;
    // trace_trace
    typedef typename trace_mesh_type::trace_mesh_type trace_trace_mesh_type;
    typedef Exporter<trace_trace_mesh_type> trace_trace_export_type;

    //--------------------------------------------------------------------------------------------------//

    auto mesh = createGMSHMesh( _mesh=new mesh_type,
                                _update=MESH_CHECK|MESH_UPDATE_FACES|MESH_UPDATE_EDGES|MESH_RENUMBER,
                                _desc=domain( _name=( boost::format( "hypercube-%1%" ) % nDim ).str() ,
                                              _addmidpoint=false, _usenames=false, _shape="hypercube",
                                              _dim=nDim, _h=meshSize,
                                              _xmin=0., _xmax=1.,
                                              _ymin=0., _ymax=1.,
                                              _zmin=0., _zmax=1.,
                                              _substructuring=true ) );


    auto backend = backend_type::build( soption( _name="backend" ) );
    auto pi = M_PI;
    auto g = sin( pi*( 2*Px()+Py()+1./4 ) )*cos( pi*( Py()-1./4 ) );

    auto Xh = space_type::New(_mesh=mesh);
    double domain_measure = integrate( _range=elements( mesh ),_expr=cst( 1. ) ).evaluate()( 0,0 );
    std::cout <<"domain_measure= " << domain_measure << std::endl;

    auto TXh = Xh->trace( markedfaces( mesh,6 ) ) ;
    double trace_measure = integrate( _range=elements( TXh->mesh() ),_expr=cst( 1. ) ).evaluate()( 0,0 );
    std::cout <<"trace_measure= " << trace_measure << std::endl;

    auto TTXh = TXh->trace();
    auto trace_trace_measure = integrate( _range=elements( TTXh->mesh() ),_expr=cst( 1. ) ).evaluate()( 0,0 );
    double trace_trace_integrate_g = integrate( _range=elements( TTXh->mesh() ),_expr=g ).evaluate()( 0,0 );
    std::cout <<"trace_trace_measure= " << trace_trace_measure << std::endl;

    // projections
    auto projection_g = vf::project( _space=Xh, _range=elements( mesh ),_expr=g );
    auto trace_projection_g = vf::project( _space=TXh,_range=elements( TXh->mesh() ),_expr=g );
    auto trace_trace_projection_g = vf::project( _space=TTXh,_range=elements( TTXh->mesh() ),_expr=g );

    // extensions
    double trace_trace_integrate = integrate( _range=elements( TTXh->mesh() ),
                                              _expr=idv( trace_trace_projection_g ) ).evaluate()( 0,0 );
    double ttmean_g = trace_trace_integrate/trace_trace_measure;
    double mean_g = trace_trace_integrate_g/trace_trace_measure;
    BOOST_MESSAGE( fmt::format( "mean_g={}, ttmean_g={}\n",mean_g,ttmean_g ) ); 


    auto zero_extension = vf::project( _space=TXh,_range=boundaryfaces( TXh->mesh() ),_expr=idv( trace_trace_projection_g ) );
    auto const_extension = vf::project( _space=TXh,_range=boundaryfaces( TXh->mesh() ),_expr=idv( trace_trace_projection_g )-ttmean_g );
    const_extension += vf::project( _space=TXh, _range=elements( TXh->mesh() ), _expr=cst( ttmean_g ) );
    //auto op_lift = opLift( _domainSpace=Xh,_backend=backend );
    auto op_lift = opLift( _domainSpace=Xh,_backend=backend,_penaldir=100. );
    auto glift = op_lift->project( _expr=idv( const_extension ), _range=markedfaces( mesh,6 ) );

    double boundary_error = integrate( _range=markedfaces( mesh,6 ), _expr=idv( glift )-idv( const_extension ) ).evaluate().norm();
    //auto laplacian_error = integrate( elements( mesh ), trace( hessv( glift ) ) ).evaluate()( 0,0 );

    double const_extention_error1 = integrate( _range=boundaryfaces( TXh->mesh() ),
                                             _expr=idv( trace_trace_projection_g )-idv( const_extension ) ).evaluate().norm();
    double const_extention_error2 = integrate( _range=elements( TXh->mesh() ), _expr=ttmean_g-idv( const_extension ) ).evaluate().norm();



    BOOST_MESSAGE( fmt::format("        boundary_error={}\n",boundary_error ) );
    BOOST_MESSAGE( fmt::format("const_extention_error1={}\n",const_extention_error1 ) );
    BOOST_MESSAGE( fmt::format("const_extention_error2={}\n", const_extention_error2 ) );

    BOOST_CHECK_SMALL( boundary_error,2e-4 );
    BOOST_CHECK_CLOSE( domain_measure, 1, 1e-10 );
    BOOST_CHECK_CLOSE( trace_measure, 1, 1e-12 );
    BOOST_CHECK_CLOSE( trace_trace_measure, 4, 1e-12 );
    BOOST_CHECK_SMALL( const_extention_error1,1e-10 );
    BOOST_CHECK_SMALL( const_extention_error2,5e-4 );


    //-------------------------------------------------------------------------------------------------------
    auto mesh3D = createGMSHMesh( _mesh=new mesh_type,
                                  _update=MESH_CHECK|MESH_UPDATE_FACES|MESH_UPDATE_EDGES|MESH_RENUMBER,
                                  _desc=domain( _name=(boost::format( "Hypercube-%1%" ) % nDim).str() ,
                                                _addmidpoint=false,
                                                _usenames=true, _shape="hypercube", _dim=nDim, _h=meshSize,
                                                _xmin=0., _xmax=1., _ymin=0., _ymax=1., _zmin=0., _zmax=1.,
                                                _substructuring=true
                                                ) );

    //auto wirebasket = createSubmesh( mesh3D, markededges(mesh3D,"WireBasket") );
    auto Xh3D = space_type::New(_mesh=mesh3D);
    auto Wh = Xh3D->wireBasket();
    FEELPP_ASSERT( Wh->mesh()->numElements() != 0 )( Wh->mesh()->numElements() ).error( "invalid wirebasket mesh" );

    saveGMSHMesh(_mesh=Wh->mesh(), _filename="wirebasket.msh");
    auto w = Wh->element();
    auto z = Wh->element();

    auto M = backend->newMatrix( _test=Wh, _trial=Wh );
    form2( _trial=Wh, _test=Wh, _matrix=M ) = integrate( _range=elements(Wh->mesh()), _expr=idt(w)*id(z) );
    w.setOnes();
    z.setOnes();
    std::cout << "measure from mass = " << M->energy( w, w ) << "\n";
    BOOST_CHECK_CLOSE( M->energy( w, w ), 12., 1e-12 );

    // test merkerToDof
    const std::string str = "WireBasket";
    auto dft = Xh3D->dof()->markerToDof(str);
    std::cout<<"nDof= "<< std::distance(dft.first,dft.second) <<"\n";
    FEELPP_ASSERT( std::distance(dft.first,dft.second) != 0 )( std::distance(dft.first,dft.second) ).error( "invalid wirebasket nDof" );

    //-------------------------------------------------------------------------------------------------------
    auto myexporter = exporter(_mesh=mesh,_name="Export" );
    myexporter->step( 0 )->add( "g", projection_g );
    myexporter->step( 0 )->add( "glift", glift );
    myexporter->save();

    auto trace_exporter = exporter(_mesh=TXh->mesh() ,_name="Trace_Export" );
    trace_exporter->step( 0 )->add( "traceg", trace_projection_g );
    trace_exporter->step( 0 )->add( "const_extension", const_extension );
    trace_exporter->save();

    auto trace_trace_exporter = exporter(_mesh=TTXh->mesh() ,_name="Trace_Trace_Export" );
    trace_trace_exporter->step( 0 )->add( "tracetrace_g", trace_trace_projection_g );
    trace_trace_exporter->save();

} // run

} //namespace test_wire_basket


/*_________________________________________________*
 * Main
 *_________________________________________________*/

FEELPP_ENVIRONMENT_NO_OPTIONS

BOOST_AUTO_TEST_SUITE( wire_basket )

BOOST_AUTO_TEST_CASE( wire_basket1 )
{
    test_wire_basket::run<2>();
}
BOOST_AUTO_TEST_SUITE_END()

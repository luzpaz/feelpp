/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
       Date: 2012-01-16

  Copyright (C) 2012 Université Joseph Fourier (Grenoble I)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file preconditioner.hpp
   \author Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
   \date 2012-01-16
 */
#ifndef __Preconditioner_H
#define __Preconditioner_H 1

#include <boost/parameter.hpp>
#include <feel/feelcore/parameter.hpp>
#include <feel/feelalg/matrixsparse.hpp>
#include <feel/feelalg/vector.hpp>

#include <feel/feelalg/enums.hpp>

namespace Feel
{
/**
 * \class Preconditioner
 * \brief base class for preconditioner
 *
 * @author Christophe Prud'homme
 * @see
 */
template<typename T>
class Preconditioner
{
public:


    /** @name Constants
     */
    //@{


    //@}

    /** @name Typedefs
     */
    //@{

    typedef Preconditioner<T> preconditioner_type;
    typedef boost::shared_ptr<Preconditioner<T> > preconditioner_ptrtype;

    typedef boost::shared_ptr<MatrixSparse<T> > sparse_matrix_ptrtype;
    typedef boost::shared_ptr<Vector<T> > vector_ptrtype;


    //@}

    /** @name Constructors, destructor
     */
    //@{

    //! default constructor
    Preconditioner();

    //! copy constructor
    Preconditioner( Preconditioner const & o )
    :
    M_matrix( o.M_matrix ),
    M_is_initialized( o.M_is_initialized ),
    M_preconditioner_type( o.M_preconditioner_type )
        {}

    //! destructor
    ~Preconditioner();

    static preconditioner_ptrtype build( BackendType = BACKEND_PETSC );

    /**
     * Initialize data structures if not done so already.
     */
    virtual void init () {};

    //@}

    /** @name Operator overloads
     */
    //@{

    //! copy operator
    Preconditioner& operator=( Preconditioner const & o)
        {
            if (this != &o )
            {
                M_matrix = o.M_matrix;
                M_is_initialized = o.M_is_initialized;
                M_preconditioner_type = o.M_preconditioner_type;
            }
            return *this;
        }
    //@}

    /** @name Accessors
     */
    //@{

    /**
     * @returns true if the data structures are
     * initialized, false otherwise.
     */
    bool initialized () const { return M_is_initialized; }

    /**
     * Computes the preconditioned vector "y" based on input "x".
     * Usually by solving Py=x to get the action of P^-1 x.
     */
    virtual void apply(const Vector<T> & x, Vector<T> & y) = 0;

    /**
     * Computes the preconditioned vector "y" based on input "x".
     * Usually by solving Py=x to get the action of P^-1 x.
     */
    void apply( vector_ptrtype const& x, vector_ptrtype& y )
        {
            this->apply( *x, *y );
        }

    /**
     * Release all memory and clear data structures.
     */
    virtual void clear () {}



    /**
     * Returns the type of preconditioner to use.
     */
    PreconditionerType type () const { return M_preconditioner_type; }




    //@}

    /** @name  Mutators
     */
    //@{

    /**
     * Sets the matrix P to be preconditioned.
     */
    void setMatrix( sparse_matrix_ptrtype  mat);

    /**
     * Sets the type of preconditioner to use.
     */
    void setType (const PreconditionerType pct);

    //@}

    /** @name  Methods
     */
    //@{


    //@}


protected:

    /**
     * The matrix P... ie the matrix to be preconditioned.
     * This is often the actual system matrix of a linear sytem.
     */
    sparse_matrix_ptrtype  M_matrix;

    /**
     * Enum statitng with type of preconditioner to use.
     */
    PreconditionerType M_preconditioner_type;

    /**
     * Flag indicating if the data structures have been initialized.
     */
    bool M_is_initialized;


};


template <typename T>
FEEL_STRONG_INLINE
Preconditioner<T>::Preconditioner ()
  :
    M_matrix(),
    M_preconditioner_type (ILU_PRECOND),
    M_is_initialized      (false)
{
}



template <typename T>
FEEL_STRONG_INLINE
Preconditioner<T>::~Preconditioner ()
{
  this->clear ();
}

template <typename T>
FEEL_STRONG_INLINE
void
Preconditioner<T>::setMatrix( sparse_matrix_ptrtype mat )
{
    M_is_initialized = false;
    M_matrix = mat;
}

template <typename T>
void
Preconditioner<T>::setType (const PreconditionerType pct)
{
  M_is_initialized = false;
  M_preconditioner_type = pct;
}

BOOST_PARAMETER_MEMBER_FUNCTION((boost::shared_ptr<Preconditioner<double> >),
                                preconditioner,
                                tag,
                                (required
                                 (matrix,(d_sparse_matrix_ptrtype))
                                 (pc,(PreconditionerType)))
                                (optional
                                 (backend,(BackendType), BACKEND_PETSC )))
{
    boost::shared_ptr<Preconditioner<double> > p = Preconditioner<double>::build(backend);
    p->setType( pc );
    p->setMatrix(matrix);
    return p;
}

} // Feel
#endif /* __Preconditioner_H */
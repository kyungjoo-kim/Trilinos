// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef ROL_GRADIENTALGORITHM_B_DEF_H
#define ROL_GRADIENTALGORITHM_B_DEF_H

namespace ROL {

template<typename Real>
GradientAlgorithm_B<Real>::GradientAlgorithm_B(ParameterList &list) {
  // Set status test
  status_->reset();
  status_->add(makePtr<StatusTest<Real>>(list));

  // Parse parameter list
  ParameterList &lslist = list.sublist("Step").sublist("Line Search");
  maxit_        = lslist.get("Function Evaluation Limit",                        20);
  alpha0_       = lslist.get("Initial Step Size",                               1.0);
  alpha0bnd_    = lslist.get("Lower Bound for Initial Step Size",              1e-4);
  useralpha_    = lslist.get("User Defined Initial Step Size",                false);
  usePrevAlpha_ = lslist.get("Use Previous Step Length as Initial Guess",     false);
  c1_           = lslist.get("Sufficient Decrease Tolerance",                  1e-4);
  maxAlpha_     = lslist.get("Maximum Step Size",                           alpha0_);
  useAdapt_     = lslist.get("Use Adaptive Step Size Selection",               true);
  rhodec_       = lslist.sublist("Line-Search Method").get("Backtracking Rate", 0.5);
  rhoinc_       = lslist.sublist("Line-Search Method").get("Increase Rate"    , 2.0);
  verbosity_    = list.sublist("General").get("Output Level",                     0);
  printHeader_  = verbosity_ > 2;
}

template<typename Real>
void GradientAlgorithm_B<Real>::initialize(Vector<Real>          &x,
                                           const Vector<Real>    &g,
                                           Objective<Real>       &obj,
                                           BoundConstraint<Real> &bnd,
                                           std::ostream &outStream) {
  const Real one(1);
  if (proj_ == nullPtr) {
    proj_ = makePtr<PolyhedralProjection<Real>>(makePtrFromRef(x),makePtrFromRef(bnd));
  }
  // Initialize data
  Algorithm_B<Real>::initialize(x,g);
  // Update approximate gradient and approximate objective function.
  Real ftol = std::sqrt(ROL_EPSILON<Real>());
  proj_->project(x);
  obj.update(x,true,state_->iter);    
  state_->value = obj.value(x,ftol); 
  state_->nfval++;
  obj.gradient(*state_->gradientVec,x,ftol);
  state_->ngrad++;
  state_->stepVec->set(x);
  state_->stepVec->axpy(-one,state_->gradientVec->dual());
  proj_->project(*state_->stepVec);
  Real fnew = state_->value;
  if (!useralpha_) {
    // Evaluate objective at P(x - g)
    obj.update(*state_->stepVec,false);
    fnew = obj.value(*state_->stepVec,ftol);
    state_->nfval++;
  }
  state_->stepVec->axpy(-one,x);
  state_->gnorm = state_->stepVec->norm();
  state_->snorm = ROL_INF<Real>();
  if (!useralpha_) {
    const Real half(0.5);
    // Minimize quadratic interpolate to compute new alpha
    Real gs    = state_->stepVec->dot(state_->gradientVec->dual());
    Real denom = (fnew - state_->value - gs);
    bool flag  = maxAlpha_ == alpha0_;
    alpha0_ = ((denom > ROL_EPSILON<Real>()) ? -half*gs/denom : alpha0bnd_);
    alpha0_ = ((alpha0_ > alpha0bnd_) ? alpha0_ : one);
    if (flag) maxAlpha_ = alpha0_;
  }
  state_->searchSize = alpha0_;
}

template<typename Real>
std::vector<std::string> GradientAlgorithm_B<Real>::run( Vector<Real>          &x,
                                                         const Vector<Real>    &g, 
                                                         Objective<Real>       &obj,
                                                         BoundConstraint<Real> &bnd,
                                                         std::ostream          &outStream ) {
  const Real one(1);
  // Initialize trust-region data
  std::vector<std::string> output;
  initialize(x,g,obj,bnd,outStream);
  Ptr<Vector<Real>> s = x.clone();
  Real ftrial(0), gs(0), ftrialP(0), alphaP(0), tol(std::sqrt(ROL_EPSILON<Real>()));
  int ls_nfval = 0;
  bool incAlpha = false;

  // Output
  output.push_back(print(true));
  if (verbosity_ > 0) outStream << print(true);

  // Compute steepest descent step
  state_->stepVec->set(state_->gradientVec->dual());
  while (status_->check(*state_)) {
    // Perform backtracking line search 
    if (!usePrevAlpha_ && !useAdapt_) state_->searchSize = alpha0_;
    state_->iterateVec->set(x);
    state_->iterateVec->axpy(-state_->searchSize,*state_->stepVec);
    proj_->project(*state_->iterateVec);
    obj.update(*state_->iterateVec,false);
    ftrial = obj.value(*state_->iterateVec,tol);
    ls_nfval = 1;
    s->set(*state_->iterateVec);
    s->axpy(-one,x);
    gs = s->dot(*state_->stepVec);
    incAlpha = (state_->value - ftrial >= -c1_*gs);
    if (verbosity_ > 1) {
      outStream << "  In GradientAlgorithm_B: Line Search"  << std::endl;
      outStream << "    Step size:                        " << state_->searchSize   << std::endl;
      outStream << "    Trial objective value:            " << ftrial               << std::endl;
      outStream << "    Computed reduction:               " << state_->value-ftrial << std::endl;
      outStream << "    Dot product of gradient and step: " << gs                   << std::endl;
      outStream << "    Sufficient decrease bound:        " << -gs*c1_              << std::endl;
      outStream << "    Number of function evaluations:   " << ls_nfval             << std::endl;
      outStream << "    Increase alpha?:                  " << incAlpha             << std::endl;
    }
    if (incAlpha && useAdapt_) {
      ftrialP = ROL_INF<Real>();
      while ( state_->value - ftrial >= -c1_*gs
           && ftrial <= ftrialP
           && state_->searchSize < maxAlpha_
           && ls_nfval < maxit_ ) {
        alphaP  = state_->searchSize;
        ftrialP = ftrial;
        state_->searchSize *= rhoinc_;
        state_->iterateVec->set(x);
        state_->iterateVec->axpy(-state_->searchSize,*state_->stepVec);
        proj_->project(*state_->iterateVec);
        obj.update(*state_->iterateVec,false);
        ftrial = obj.value(*state_->iterateVec,tol);
        ls_nfval++;
        s->set(*state_->iterateVec);
        s->axpy(-one,x);
        gs = s->dot(*state_->stepVec);
        if (verbosity_ > 1) {
          outStream << std::endl;
          outStream << "    Step size:                        " << state_->searchSize   << std::endl;
          outStream << "    Trial objective value:            " << ftrial               << std::endl;
          outStream << "    Computed reduction:               " << state_->value-ftrial << std::endl;
          outStream << "    Dot product of gradient and step: " << gs                   << std::endl;
          outStream << "    Sufficient decrease bound:        " << -gs*c1_              << std::endl;
          outStream << "    Number of function evaluations:   " << ls_nfval             << std::endl;
        }
      }
      if (state_->value - ftrial < -c1_*gs || ftrial > ftrialP) {
        ftrial = ftrialP;
        state_->searchSize = alphaP;
        state_->iterateVec->set(x);
        state_->iterateVec->axpy(-state_->searchSize,*state_->stepVec);
        proj_->project(*state_->iterateVec);
        obj.update(*state_->iterateVec,false);
        s->set(*state_->iterateVec);
        s->axpy(-one,x);
      }
    }
    else {
      while ( state_->value - ftrial < -c1_*gs && ls_nfval < maxit_ ) {
        state_->searchSize *= rhodec_;
        state_->iterateVec->set(x);
        state_->iterateVec->axpy(-state_->searchSize,*state_->stepVec);
        proj_->project(*state_->iterateVec);
        obj.update(*state_->iterateVec,false);
        ftrial = obj.value(*state_->iterateVec,tol);
        ls_nfval++;
        s->set(*state_->iterateVec);
        s->axpy(-one,x);
        gs = s->dot(*state_->stepVec);
        if (verbosity_ > 1) {
          outStream << std::endl;
          outStream << "    Step size:                        " << state_->searchSize   << std::endl;
          outStream << "    Trial objective value:            " << ftrial               << std::endl;
          outStream << "    Computed reduction:               " << state_->value-ftrial << std::endl;
          outStream << "    Dot product of gradient and step: " << gs                   << std::endl;
          outStream << "    Sufficient decrease bound:        " << -gs*c1_              << std::endl;
          outStream << "    Number of function evaluations:   " << ls_nfval             << std::endl;
        }
      }
    }
    state_->nfval += ls_nfval;

    // Compute norm of step
    state_->stepVec->set(*s);
    state_->snorm = state_->stepVec->norm();

    // Update iterate
    x.set(*state_->iterateVec);

    // Compute new value and gradient
    state_->iter++;
    state_->value = ftrial;
    obj.update(x,true,state_->iter);
    obj.gradient(*state_->gradientVec,x,tol);
    state_->ngrad++;

    // Compute steepest descent step
    state_->stepVec->set(state_->gradientVec->dual());

    // Compute projected gradient norm
    s->set(x); s->axpy(-one,*state_->stepVec);
    proj_->project(*s);
    s->axpy(-one,x);
    state_->gnorm = s->norm();

    // Update Output
    output.push_back(print(printHeader_));
    if (verbosity_ > 0) outStream << print(printHeader_);
  }
  output.push_back(Algorithm_B<Real>::printExitStatus());
  if (verbosity_ > 0) outStream << Algorithm_B<Real>::printExitStatus();
  return output;
}

template<typename Real>
std::string GradientAlgorithm_B<Real>::printHeader( void ) const {
  std::stringstream hist;
  if (verbosity_ > 1) {
    hist << std::string(109,'-') << std::endl;
    hist << "Projected gradient descent";
    hist << " status output definitions" << std::endl << std::endl;
    hist << "  iter     - Number of iterates (steps taken)" << std::endl;
    hist << "  value    - Objective function value" << std::endl;
    hist << "  gnorm    - Norm of the gradient" << std::endl;
    hist << "  snorm    - Norm of the step (update to optimization vector)" << std::endl;
    hist << "  alpha    - Line search step length" << std::endl;
    hist << "  #fval    - Cumulative number of times the objective function was evaluated" << std::endl;
    hist << "  #grad    - Cumulative number of times the gradient was computed" << std::endl;
    hist << std::string(109,'-') << std::endl;
  }

  hist << "  ";
  hist << std::setw(6)  << std::left << "iter";
  hist << std::setw(15) << std::left << "value";
  hist << std::setw(15) << std::left << "gnorm";
  hist << std::setw(15) << std::left << "snorm";
  hist << std::setw(15) << std::left << "alpha";
  hist << std::setw(10) << std::left << "#fval";
  hist << std::setw(10) << std::left << "#grad";
  hist << std::endl;
  return hist.str();
}

template<typename Real>
std::string GradientAlgorithm_B<Real>::printName( void ) const {
  std::stringstream hist;
  hist << std::endl << "Projected Gradient Descent with Backtracking Line Search" << std::endl;
  return hist.str();
}

template<typename Real>
std::string GradientAlgorithm_B<Real>::print( const bool print_header ) const {
  std::stringstream hist;
  hist << std::scientific << std::setprecision(6);
  if ( state_->iter == 0 ) {
    hist << printName();
  }
  if ( print_header ) {
    hist << printHeader();
  }
  if ( state_->iter == 0 ) {
    hist << "  ";
    hist << std::setw(6)  << std::left << state_->iter;
    hist << std::setw(15) << std::left << state_->value;
    hist << std::setw(15) << std::left << state_->gnorm;
    hist << std::endl;
  }
  else {
    hist << "  ";
    hist << std::setw(6)  << std::left << state_->iter;
    hist << std::setw(15) << std::left << state_->value;
    hist << std::setw(15) << std::left << state_->gnorm;
    hist << std::setw(15) << std::left << state_->snorm;
    hist << std::setw(15) << std::left << state_->searchSize;
    hist << std::setw(10) << std::left << state_->nfval;
    hist << std::setw(10) << std::left << state_->ngrad;
    hist << std::endl;
  }
  return hist.str();
}

} // namespace ROL

#endif

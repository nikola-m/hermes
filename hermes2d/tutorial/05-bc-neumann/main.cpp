#define HERMES_REPORT_INFO
#include "hermes2d.h"

// This example shows how to define Neumann boundary conditions. In addition,
// you will see how a Filter is used to visualize gradient of the solution
//
// PDE: Poisson equation -Laplace u = f, where f = CONST_F
//
// BC: u = 0 on Gamma_inner (edges meeting at the re-entrant corner),
//     du/dn = CONST_GAMMA_BOTTOM on Gamma_bottom,
//     du/dn = CONST_GAMMA_OUTER on Gamma_outer,
//     du/dn = CONST_GAMMA_LEFT on Gamma_left.
//
// You can play with the parameters below. For most choices of the four constants,
// the solution has a singular (infinite) gradient at the re-entrant corner.
// Therefore we visualize not only the solution but also its gradient.

const int P_INIT = 4;                             // Initial polynomial degree in all elements.
const int CORNER_REF_LEVEL = 12;                  // Number of mesh refinements towards the re-entrant corner.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
                                                  // SOLVER_PARDISO, SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.

// Boundary markers.
const int BDY_BOTTOM = 1, BDY_OUTER = 2, BDY_LEFT = 3, BDY_INNER = 4;

// Problem parameters.
const double CONST_F = -1.0;                      // Right-hand side.
const double CONST_GAMMA_BOTTOM = -0.5;           // Outer normal derivative on Gamma_1.
const double CONST_GAMMA_OUTER = 1.0;             // Outer normal derivative on Gamma_2.
const double CONST_GAMMA_LEFT = -0.5;             // Outer normal derivative on Gamma_3.

// Weak forms.
#include "forms.cpp"

int main(int argc, char* argv[])
{
  // Load the mesh.
  Mesh mesh;
  H2DReader mloader;
  mloader.load("domain.mesh", &mesh);

  // Perform initial mesh refinements.
  mesh.refine_towards_vertex(3, CORNER_REF_LEVEL);

  // Enter boundary markers.
  BCTypes bc_types;
  bc_types.add_bc_dirichlet(BDY_INNER);
  bc_types.add_bc_neumann(Hermes::Tuple<int>(BDY_BOTTOM, BDY_OUTER, BDY_LEFT));

  // Enter Dirichlet boudnary values.
  BCValues bc_values;
  bc_values.add_zero(BDY_INNER);

  // Create an H1 space with default shapeset.
  H1Space space(&mesh, &bc_types, &bc_values, P_INIT);
  int ndof = Space::get_num_dofs(&space);
  info("ndof = %d", ndof);

  // Initialize the weak formulation.
  WeakForm wf;
  wf.add_matrix_form(callback(bilinear_form));
  wf.add_vector_form(callback(linear_form));
  wf.add_vector_form_surf(callback(linear_form_surf_bottom), BDY_BOTTOM);
  wf.add_vector_form_surf(callback(linear_form_surf_outer), BDY_OUTER);
  wf.add_vector_form_surf(callback(linear_form_surf_left), BDY_LEFT);

  // Initialize the FE problem.
  bool is_linear = true;
  DiscreteProblem dp(&wf, &space, is_linear);

  // Set up the solver, matrix, and rhs according to the solver selection.
  SparseMatrix* matrix = create_matrix(matrix_solver);
  Vector* rhs = create_vector(matrix_solver);
  Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);

  // Initialize the solution.
  Solution sln;

  // Assemble the stiffness matrix and right-hand side vector.
  info("Assembling the stiffness matrix and right-hand side vector.");
  dp.assemble(matrix, rhs);

  // Solve the linear system and if successful, obtain the solution.
  info("Solving the matrix problem.");
  if(solver->solve())
    Solution::vector_to_solution(solver->get_solution(), &space, &sln);
  else
    error ("Matrix solver failed.\n");

  // Visualize the approximation.
  ScalarView view("Solution", new WinGeom(0, 0, 440, 350));
  view.show(&sln);

  // Compute and show gradient magnitude.
  // (Note that the gradient at the re-entrant
  // corner needs to be truncated for visualization purposes.)
  ScalarView gradview("Gradient", new WinGeom(450, 0, 400, 350));
  MagFilter grad(Hermes::Tuple<MeshFunction *>(&sln, &sln), Hermes::Tuple<int>(H2D_FN_DX, H2D_FN_DY));
  gradview.show(&grad);

  // Wait for the views to be closed.
  View::wait();

  // Clean up.
  delete solver;
  delete matrix;
  delete rhs;

  return 0;
}

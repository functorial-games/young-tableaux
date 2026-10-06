# Tom Leinster — general self-similarity

Research notes on Tom Leinster's 2004 self-similarity series and the later merged paper.

## Original sources

- **General self-similarity: an overview** (2004)  
  arXiv: https://arxiv.org/abs/math/0411343
- **A general theory of self-similarity I** (2004)  
  arXiv: https://arxiv.org/abs/math/0411344
- **A general theory of self-similarity II: recognition** (2004)  
  arXiv: https://arxiv.org/abs/math/0411345
- **A general theory of self-similarity** (2010 preprint; published 2011)  
  arXiv: https://arxiv.org/abs/1010.4474  
  DOI: https://doi.org/10.1016/j.aim.2010.10.009
- **Author's self-similarity bibliography**  
  https://webhomes.maths.ed.ac.uk/~tl/

Leinster's author page says that the 2010 paper supersedes parts I and II. The 2004 overview remains useful because it explains the motivation and examples informally.

## 1. Overview: what Leinster means by self-similarity

The central distinction is between **local** and **global** self-similarity.

- Local self-similarity says that small patterns recur throughout an object and at different scales.
- Global self-similarity says that the whole object can be described as a gluing of copies of itself, or of members of a finite/countable family of related objects.

Leinster works with the global version. The emphasis is not on an object sitting inside Euclidean space, but on an intrinsic recursive description.

A family of spaces can satisfy several simultaneous gluing equations. Schematically,

```
X₁ = gluing of copies of X₁, X₂, ...
X₂ = gluing of copies of X₁, X₂, ...
...
```

The point is that the equations themselves can determine a canonical space. The desired object is not merely a fixed point of the recursion; it is the **universal solution**.

The motivating example is Peter Freyd's characterization of the interval. The unit interval is two copies of itself glued end to end. Leinster packages this as a coalgebra and characterizes the interval as a terminal coalgebra for the corresponding gluing operation.

A useful slogan:

> self-similarity is treated as a system of recursive gluing equations, and the intended object is the universal solution of those equations.

This is much stronger than observing that a picture happens to resemble a smaller copy of itself.

## 2. Part I: the formal machinery

### Self-similarity systems

A self-similarity system consists of:

- a small category `A`, indexing the family of spaces and the maps among them;
- a finite nondegenerate two-sided module
  `M : Aᵒᵖ × A → Set`,
  encoding which pieces appear in the gluing formula for each object.

From `M` one gets an endofunctor

```
M ⊗ − : [A, Set] → [A, Set]
```

and similarly in topological spaces.

The analogy with linear algebra is explicit. A matrix `M` turns a vector equation into

```
x = Mx.
```

Here the unknowns are spaces rather than scalars, and the analogue is

```
X ≅ M ⊗ X.
```

The tensor/coend is doing the bookkeeping for "take these pieces and glue them according to the incidence data."

### Fixed point versus universal solution

A fixed point is not enough. Many different objects can satisfy the same formal equation.

Leinster therefore defines the universal solution as the **terminal `M`-coalgebra**. Lambek's lemma then guarantees that its structure map is an isomorphism, so every universal solution is a fixed point, but not every fixed point is universal.

This distinction is important for interactive mathematics: a recursive rule may admit many accidental configurations, while the universal property identifies the canonical one.

### Nondegeneracy

Without a nondegeneracy condition, gluing can collapse information and produce trivial solutions. The theory therefore restricts to nondegenerate functors and nondegenerate modules.

Conceptually, nondegeneracy prevents different pieces from being identified for reasons not specified by the gluing data.

### Solvability condition S

Part I gives an explicit condition, called **S**, on a self-similarity system. It has two coherence clauses (S1 and S2) ensuring that compatible infinite recursive descriptions can be reconciled.

The important result is:

- condition S is sufficient for the universal solution to exist;
- the appendix proves that it is also necessary.

So existence is not merely asserted abstractly; the paper gives a checkable criterion.

### Explicit construction

For an object `a`, Leinster considers infinite chains

```
⋯ → a₂ → a₁ → a₀ = a
```

whose arrows are module elements describing successively finer pieces.

The set `I(a)` is built from equivalence classes / connected components of these infinite recursive descriptions. Removing the first step of a chain supplies the coalgebra structure.

This construction is the conceptual heart of the paper: **a point of the final space can be represented by an infinite descent through nested pieces**.

In the Freyd interval example, those infinite descents become binary expansions.

### Topology

Finite prefixes determine basic closed sets: each contains the points admitting a recursive description with that prefix. These sets generate the topology through closed sets; a point can have more than one recursive description.

This provides a topology compatible with the recursion, and the resulting coalgebra is proved terminal both in `Set` and in `Top`.

One subtle warning in the paper: the universal solution is not, in general, obtained simply by taking the naive inverse limit of the finite approximations. The equivalence relation among infinite descriptions matters.

## 3. Part II: recognition

Part II reverses the problem.

Part I says: given the recursive system, construct the universal solution.

Part II says: given a candidate recursive space, how can we recognize that it is the universal solution?

### Precise Recognition Theorem

For a nondegenerate fixed point `J`, Part II, Theorem 1.4 characterizes universality by three requirements: `J` is occupied, each component is compact, and every allowed infinite descent has an intersection containing at most one point. Occupied means that a component of `J` is nonempty whenever an infinite recursive description exists for that component; it prevents an empty candidate from passing the shrinking test vacuously.

Equivalently, each component admits a compatible metric such that, for every positive size bound, one depth makes every recursive piece at that depth smaller than the bound. The depth works uniformly over all allowed pieces in that component.

This is the mathematically useful bridge between abstract coalgebra and ordinary geometric intuition.

### Crude Recognition Theorem

When the indexing category is finite, a convenient sufficient condition is:

- each component space is nonempty, compact and equipped with a compatible metric;
- each map inserting a smaller piece into a larger one is a contraction.

These hypotheses apply to a nondegenerate fixed point, as in Part II, Corollary 1.5. Finiteness supplies one contraction bound below 1 for the whole family.

Then the fixed point is automatically the universal solution.

This recovers the familiar "smaller and smaller copies" picture without making that picture the definition.

## 4. Examples in Part II

Leinster works through a broad range of examples.

### Interval

`[0,1]` is two copies of itself glued end to end.

The two nontrivial maps are

```
t ↦ t/2
t ↦ (t+1)/2
```

and are contractions, so the recognition theorem identifies the interval as the universal solution.

### Circle

The circle is handled by gluing intervals while identifying endpoints. This is useful because it shows that overlaps and quotient identifications belong naturally in the framework.

### Cantor set

For a finite alphabet `M`, the infinite product `M^ℕ` is the universal solution of the corresponding shift recursion.

For two symbols this is the usual Cantor set.

### Products and cubes

Products of self-similarity systems give product spaces. In particular, cubes arise from products of the interval system.

### Simplices

The standard simplices `Δⁿ` are universal solutions for systems expressing barycentric subdivision.

Leinster also describes edgewise subdivision. The same underlying simplex therefore carries more than one self-similarity structure.

This is an important distinction: **self-similarity is structure on a space, not merely a property of the underlying topological space.**

### Iterated function systems

Under suitable overlap conditions, ordinary contractive IFS attractors fit the recognition theorem. Leinster's framework is broader because it does not begin with an ambient Euclidean space.

## 5. Which spaces are self-similar?

Part II proves a striking classification:

> A topological space admits at least one self-similarity structure of Leinster's kind iff it is compact and metrizable.

The construction may require countably many simultaneous equations even though each individual gluing equation is finite.

So the notion is extremely broad. The interesting information is often not that a space is self-similar, but **which self-similarity structure it carries**.

## 6. Cantor-set consequences

The framework gives conceptual proofs of classical facts.

- Every nonempty discretely self-similar space is a retract of the Cantor set.
- Every nonempty self-similar space is a quotient of a discretely self-similar space.
- Therefore every nonempty compact metrizable space is a quotient of the Cantor set.
- A perfect discretely self-similar space is either empty or homeomorphic to the Cantor set.

The Cantor set appears as a universal source of discrete recursive descriptions.

## 7. What is especially useful for interactive mathematics

Several ideas are directly reusable when designing mathematical toys.

### The interaction should expose the recursive law

A good self-similarity interaction should not merely animate successive copies. The user's action should perform the structural operation that regenerates the same problem.

Examples:

- cut;
- duplicate;
- glue;
- rotate;
- rescale;
- choose a nested piece and make it the new whole.

### Infinite descent can be represented by finite interaction

The user never needs to carry out infinitely many steps. A finite gesture establishes the recurrence, and repeating the same gesture makes the recursion visible.

This matches Leinster's use of finite prefixes of an infinite recursive address.

### Exact self-similarity is more interesting than approximate resemblance

For the APK ideas involving paper rectangles or the pentagram, the success condition can be geometric coincidence: after the recursive operation, the new figure exactly matches the old shape up to scale and rotation.

That is a stronger experience than displaying the value of a ratio.

## 8. Important limitation for the √2 and φ APK ideas

Leinster's published theory here is deliberately **topological and intrinsic**. It abstracts away from:

- Euclidean lengths;
- angles;
- scale factors;
- ambient embeddings.

Therefore the A-series paper rectangle and golden-ratio pentagram are not direct metric consequences of this theory.

Their most important features are metric:

- the A-series rectangle reproduces its shape after halving/doubling and a 90° rotation precisely because its aspect ratio is √2;
- the pentagram reproduces its length ratios through nested similar pentagons, producing φ.

Leinster's framework is still a good structural guide: encode the repeated operation, the recursive pieces, and the fact that the same problem reappears. But a formal treatment of the √2 or φ scale factors would need metric/similarity data in addition to the topological gluing structure.

## 9. Short takeaway

The most useful idea from the series is not "fractals are recursive."

It is:

> Specify a finite or countable family of gluing rules. Regard a point as an infinite compatible descent through those rules. Then characterize the intended space by a universal property, and recognize it geometrically when nested pieces shrink to points.

That viewpoint is unusually well suited to interactions where the user repeatedly performs one operation and watches the same mathematical situation regenerate at a new scale.

Clutter = imports.gi.Clutter;

GObject = imports.gi.GObject;

is_decoration = (actor) => {
    if(!actor)
        return false;
    let children = actor.get_children();
    if(children.length === 0)
        return false;
    return GObject.type_name(children[0]) === "ClutterWaylandSurface";
}

deep_clone = (actor) => {
    let clone;
    if(is_decoration(actor)) {
        clone = new Clutter.Clone({source: actor});
        clone.set_position(actor.x, actor.y);
    } else {
        // clone = new actor.constructor();
        clone = new Clutter.Actor();
        clone.set_position(actor.x, actor.y);
        clone.set_size(actor.width, actor.height);

        actor.get_children().forEach((child) => {
            clone.add_child(deep_clone(child));
        });
    }

    return clone;
}

create_minimap = (workspace) => {
    let minimap = deep_clone(workspace);

    let scale = 0.2;

    minimap.set_size(scroll.width/scale, scroll.height/scale);
    minimap.set_scale(scale, scale);

    return minimap;
}

() => {
    minimap = create_minimap(scroll)
    stage.add_child(minimap)
    minimap.destroy()
}


stage = Clutter.Stage.get_default();

workspace = new Clutter.Actor();

stage.add_child(workspace);

red = Clutter.Color.from_string("red")[1];
grey = Clutter.Color.from_string("grey")[1];
cyan = Clutter.Color.from_string("cyan")[1];
blue = Clutter.Color.from_string("blue")[1];
workspace.set_background_color(red)


scroll = new Clutter.ScrollActor();
scroll.set_easing_duration(250);
stage.connect("activate", (stage) => {
    workspace.width = stage.width;
    workspace.height = stage.height;
    scroll.set_position(10,10);
    scroll.width = workspace.width - 20;
    scroll.height = workspace.height - 20;
})

scroll.set_scroll_mode(Clutter.ScrollMode.HORIZONTALLY);

workspace.add_child(scroll);

boxlayout = new Clutter.BoxLayout();

scroll.set_layout_manager(boxlayout);
//: t

scroll.set_background_color(blue);



margin = new Clutter.Margin({
    left: 4,
    right: 4,
    top: 4,
    bottom: 4
});


() => {
    actor.grab_key_focus()
    actor = scroll.get_children()[2].get_children()[0]
    [_, vertex.x, absY] = stage.transform_stage_point(actor.x, actor.y)
    [_, vertex.x, absY, _] = scroll.apply_relative_transform_to_point(stage, new Clutter.Vertex({
        x: actor.x, y: actor.y, z: 0}));
}


focus = (actor) => {
    actor.grab_key_focus();
    let decoration = actor.get_parent();
    decoration.set_background_color(cyan);
    let overlap = 20;
    if (decoration === scroll.get_first_child() ||
        decoration === scroll.get_last_child()) {
        overlap = 0;
    }
    let absPosition = decoration
        .apply_relative_transform_to_point(scroll,
                                           new Clutter.Vertex({x: 0, y: 0, z: 0}));
    if (absPosition.x < 0) {
        print("scroll left: " + (decoration.x));
        scroll.scroll_to_point(new Clutter.Point({ x: decoration.x - overlap,
                                                   y: decoration.y }));
    } else if (absPosition.x + decoration.width + 20 > 0 + stage.width ) {
        print("scroll right: " + (decoration.x + scroll.width));
        scroll.scroll_to_point(new Clutter.Point({
            x: decoration.x + decoration.width - scroll.width + overlap,
            y: decoration.y }));
    }
};

focusWrapper = (actor) => {
    focus(actor);
};

leave = (actor) => {
    let decoration = actor.get_parent();
    decoration.set_background_color(grey);
}

leaveWrapper = (actor) => {
    leave(actor);
};

stage.connect("new-window", (stage, actor) => {
    print(actor.toString());
    let decoration = new Clutter.Actor();
    decoration.set_background_color(grey);
    actor.set_margin(margin)
    decoration.add_child(actor);
    scroll.add_child(decoration);
    actor.set_size(500,700);
    actor.connect("key-focus-out", leaveWrapper)
    actor.connect("button-press-event", focusWrapper);
})
//: 268

scroll.get_children().forEach((decoration)=> {
    let actor = decoration.get_children()[0];
    actor.set_size(300,700);
})

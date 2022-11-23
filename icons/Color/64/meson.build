icons_dir = size + 'x' + size

if have_vector_icons
  icon_dialog_question = custom_target('icon-dialog-question',
    input : [ '../scalable/dialog-question.svg' ],
    output: [ 'dialog-question.png' ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size,
    ],
    install: false,
  )
  icon_wilber_eek = custom_target('icon-wilber-eek',
    input : [ '../scalable/ligma-wilber-eek.svg' ],
    output: [ 'ligma-wilber-eek.png' ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size,
    ],
    install: false,
  )
else
  foreach icon : source_icons
    icon_name = icon + '.png'
    input_path = '../scalable/' + icon + '.svg'

    icon_tgt = custom_target(icon_name,
      input : [ input_path ],
      output: [ icon_name ],
      command: [
        colorsvg2png, '@INPUT@', '@OUTPUT@', size,
      ],
      install: true,
      install_dir: ligmaiconsdir / theme / icons_dir / 'apps',
    )

    if icon == 'dialog-question'
      icon_dialog_question = icon_tgt
    elif icon == 'ligma-wilber-eek'
      icon_wilber_eek = icon_tgt
    endif
  endforeach
endif
